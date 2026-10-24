// --stage vsr-image (docs/gpu/ENH-047.md § 5) : une image BMP agrandie par le RTX Video SDK
// (pont d'ENH-040), N appels chronometres sur la meme image, sortie BMP 24 bits.

#include "enh/bench/bmp.hpp"
#include "enh/pipeline/gpu/cuda_raii.hpp"
#include "enh/pipeline/gpu/cuda_rgb_image.hpp"
#include "enh/pipeline/rtxvideo/rtxvideo_upscaler.hpp"

#include "stages.hpp"

#include <cuda_runtime_api.h>

#include <chrono>
#include <filesystem>
#include <utility>
#include <vector>

namespace enh::bench::gpu {

namespace {

using Clock = std::chrono::steady_clock;
using pipeline::fail;
using pipeline::PipelineErrorCode;
using pipeline::Result;
using pipeline::RgbFormat;
using pipeline::gpu::checkCuda;
using pipeline::gpu::CudaRgbImage;

template <typename T> enh::Unexpected<pipeline::PipelineError> forward(Result<T>& r) {
    return enh::Unexpected<pipeline::PipelineError>(std::move(r.error()));
}

/// Evenement CUDA AVEC chronometrage (cudaEventCreate -> cudaEventDestroy via CudaEvent).
Result<pipeline::gpu::CudaEvent> makeTimingEvent() {
    cudaEvent_t e = nullptr;
    if (auto r = checkCuda(cudaEventCreate(&e), "cudaEventCreate"); !r) {
        return forward(r);
    }
    return pipeline::gpu::CudaEvent(e);
}

/// Duree d'une operation mise en file sur `stream`, mesuree par evenements (une synchro).
template <typename F>
Result<double> gpuMs(cudaStream_t stream, cudaEvent_t e0, cudaEvent_t e1, F&& op) {
    if (auto r = checkCuda(cudaEventRecord(e0, stream), "cudaEventRecord"); !r) {
        return forward(r);
    }
    if (auto r = op(); !r) {
        return forward(r);
    }
    if (auto r = checkCuda(cudaEventRecord(e1, stream), "cudaEventRecord"); !r) {
        return forward(r);
    }
    if (auto r = checkCuda(cudaEventSynchronize(e1), "cudaEventSynchronize"); !r) {
        return forward(r);
    }
    float ms = 0.0F;
    if (auto r = checkCuda(cudaEventElapsedTime(&ms, e0, e1), "cudaEventElapsedTime"); !r) {
        return forward(r);
    }
    return static_cast<double>(ms);
}

Result<std::filesystem::path> benchNgxDir() {
    std::error_code ec;
    std::filesystem::path dir =
        std::filesystem::temp_directory_path(ec) / L"enhance-bench" / L"ngx";
    if (ec) {
        return fail(PipelineErrorCode::WriteDenied, "temp directory unavailable", ec.value());
    }
    std::filesystem::create_directories(dir, ec);
    if (ec) {
        return fail(PipelineErrorCode::WriteDenied, "cannot create the NGX data directory",
                    ec.value());
    }
    return dir;
}

} // namespace

Result<void> runVsrImageStage(const BenchArgs& args, DxgiCudaProbe& probe, BenchReport& report,
                              OutputFile& output) {
    // 1. Lecture durcie du BMP (hors chronometrage).
    auto file = readFileBounded(args.input, kMaxBmpFileBytes);
    if (!file) {
        return forward(file);
    }
    report.input.size_bytes = file->size();
    auto image = decodeBmp(*file);
    if (!image) {
        return forward(image);
    }
    file->clear();
    file->shrink_to_fit();
    const pipeline::Size in_size = image->size_px;
    report.input.codec = "bmp";
    report.input.width = in_size.width;
    report.input.height = in_size.height;
    report.input.pix_fmt = "bgr24";
    const pipeline::Size out_size = fitInside(in_size, args.size);
    if (out_size.width < in_size.width || out_size.height < in_size.height) {
        return fail(PipelineErrorCode::InvalidParams, "--size " + std::to_string(args.size.width) +
                                                          "x" + std::to_string(args.size.height) +
                                                          " is smaller than the input image");
    }
    const RgbFormat format = args.bits == 10 ? RgbFormat::Abgr10 : RgbFormat::Argb8;
    const std::vector<std::uint32_t> host_in =
        format == RgbFormat::Abgr10 ? toAbgr10(*image) : toArgb8(*image);
    std::vector<std::uint32_t> host_out(std::size_t{out_size.width} * out_size.height);

    // 2. Flux, evenements, contexte.
    auto ctx = pipeline::gpu::initPrimaryContext(0);
    if (!ctx) {
        return forward(ctx);
    }
    auto stream_owner = pipeline::gpu::makeStream();
    if (!stream_owner) {
        return forward(stream_owner);
    }
    cudaStream_t stream = stream_owner->get();
    auto ev0 = makeTimingEvent();
    auto ev1 = makeTimingEvent();
    if (!ev0) {
        return forward(ev0);
    }
    if (!ev1) {
        return forward(ev1);
    }
    auto ngx_dir = benchNgxDir();
    if (!ngx_dir) {
        return forward(ngx_dir);
    }
    const auto quality = static_cast<pipeline::UpscaleQuality>(args.quality);

    double upload_ms = 0.0;
    double vsr_gpu_ms = 0.0;
    double download_ms = 0.0;
    for (std::uint32_t rep = 0; rep < args.repeat; ++rep) {
        const bool last = rep + 1 == args.repeat;
        if (auto r = checkCuda(cudaDeviceSynchronize(), "cudaDeviceSynchronize"); !r) {
            return r;
        }
        const double before_io = probe.processMib();
        auto in_img = CudaRgbImage::create(in_size, format);
        if (!in_img) {
            return forward(in_img);
        }
        auto out_img = CudaRgbImage::create(out_size, format);
        if (!out_img) {
            return forward(out_img);
        }
        auto up_ms =
            gpuMs(stream, ev0->get(), ev1->get(), [&] { return in_img->upload(host_in, stream); });
        if (!up_ms) {
            return forward(up_ms);
        }
        upload_ms += *up_ms;

        const VramSample rest = probe.sample().value_or(VramSample{});
        if (rep == 0) {
            report.vram_io_mib = rest.process_mib - before_io;
            report.vram_rest_mib = rest.process_mib;
            report.vram_device_used_rest_mib = rest.device_used_mib;
        }

        Result<void> run{};
        VramPeaks peaks;
        double wall_s = 0.0;
        {
            VramMonitor monitor(probe, std::chrono::milliseconds(args.sample_ms));
            run = [&]() -> Result<void> {
                pipeline::rtxvideo::RtxVideoUpscalerConfig cfg;
                cfg.ngx_data_dir = *ngx_dir;
                cfg.cu_context = *ctx;
                cfg.cu_stream = stream;
                auto up = pipeline::rtxvideo::RtxVideoUpscaler::create(cfg);
                if (!up) {
                    return forward(up);
                }
                if (report.vsr_feature.empty()) {
                    const auto& prov = (*up)->provenance();
                    report.vsr_feature =
                        std::string(pipeline::rtxvideo::toString(prov.source)) + " " + prov.version;
                }
                const pipeline::UpscaleRequest req =
                    pipeline::gpu::makeUpscaleRequest(*in_img, *out_img, quality);
                for (std::uint32_t i = 0; i < args.warmup; ++i) {
                    if (auto r = (*up)->upscale(req); !r) {
                        return r;
                    }
                }
                if (auto r = checkCuda(cudaStreamSynchronize(stream), "cudaStreamSynchronize");
                    !r) {
                    return r;
                }
                // Boucle chronometree : aucune synchronisation entre deux appels.
                const auto t0 = Clock::now();
                auto ms = gpuMs(stream, ev0->get(), ev1->get(), [&]() -> Result<void> {
                    for (std::uint32_t i = 0; i < args.iterations; ++i) {
                        if (auto r = (*up)->upscale(req); !r) {
                            return r;
                        }
                    }
                    return {};
                });
                wall_s = std::chrono::duration<double>(Clock::now() - t0).count();
                if (!ms) {
                    return forward(ms);
                }
                vsr_gpu_ms += *ms;
                return {};
            }(); // l'upscaler (effet + runtime NGX) est detruit ici, avant l'arret de la mesure
            peaks = monitor.stop();
        }
        report.vram_peak_mib = std::max(report.vram_peak_mib, peaks.process_peak_mib);
        report.vram_device_used_peak_mib =
            std::max(report.vram_device_used_peak_mib, peaks.device_used_peak_mib);
        report.vram_samples += peaks.samples;
        if (!run) {
            return run;
        }
        report.frames_in += std::uint64_t{args.warmup} + args.iterations;
        report.frames_warmup += args.warmup;
        report.frames_out += args.iterations;
        report.wall_s += wall_s;

        if (last) {
            auto down_ms = gpuMs(stream, ev0->get(), ev1->get(),
                                 [&] { return out_img->download(host_out, stream); });
            if (!down_ms) {
                return forward(down_ms);
            }
            download_ms += *down_ms;
        }
        // Images liberees avant le releve « apres » (comparaison des repetitions).
        *in_img = CudaRgbImage{};
        *out_img = CudaRgbImage{};
        (void)cudaDeviceSynchronize();
        report.repeats.push_back({.frames_out = args.iterations,
                                  .wall_s = wall_s,
                                  .vram_peak_mib = peaks.process_peak_mib,
                                  .vram_after_mib = probe.processMib()});
    }
    report.stages = {
        {.name = "upload", .ms_total = upload_ms, .frames = args.repeat},
        {.name = "vsr", .ms_total = vsr_gpu_ms, .frames = report.frames_out},
        {.name = "download", .ms_total = download_ms, .frames = 1},
    };

    // 3. Sortie BMP 24 bits (le chemin 10 bits est ramene a 8 bits pour le fichier).
    const RgbImage result = format == RgbFormat::Abgr10 ? fromAbgr10(host_out, out_size)
                                                        : fromArgb8(host_out, out_size);
    auto bmp = encodeBmp24(result);
    if (!bmp) {
        return forward(bmp);
    }
    if (auto r = output.replaceContent(*bmp); !r) {
        return r;
    }
    report.output_name = fileNameUtf8(*args.output);
    return {};
}

} // namespace enh::bench::gpu
