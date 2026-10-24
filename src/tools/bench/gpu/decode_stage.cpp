// --stage decode (docs/gpu/ENH-047.md § 2-3) : boucle demux -> decodage d'ENH-008, chronometree.
// Une seule synchronisation GPU a la frontiere de chauffe et une a la fin (hors boucle chaude).

#include "enh/pipeline/core/win32_file.hpp"
#include "enh/pipeline/ffmpeg/ffmpeg_demuxer.hpp"
#include "enh/pipeline/gpu/cuda_frame_store.hpp"
#include "enh/pipeline/gpu/cuda_raii.hpp"
#include "enh/pipeline/gpu/nvdec_decoder.hpp"

#include "stages.hpp"

#include <cuda_runtime_api.h>

#include <chrono>
#include <optional>
#include <string>
#include <utility>

namespace enh::bench::gpu {

namespace {

using Clock = std::chrono::steady_clock;
using pipeline::fail;
using pipeline::PipelineErrorCode;
using pipeline::Result;

/// Recoit les images (et les rend aussitot au pool) ; demarre le chronometre apres la chauffe.
class TimingSink final : public pipeline::IFrameSink {
public:
    TimingSink(std::uint64_t warmup, cudaStream_t stream) noexcept
        : warmup_(warmup), stream_(stream) {}

    Result<void> push(pipeline::VideoFrame&& frame, std::stop_token /*st*/) override {
        if (count_ == 0) {
            format_ = frame.desc.format;
            size_ = frame.desc.size_px;
        }
        ++count_;
        if (!start_ && count_ == warmup_) {
            // Frontiere de chauffe : une synchronisation, une seule fois par passe.
            if (auto r = pipeline::gpu::checkCuda(cudaStreamSynchronize(stream_),
                                                  "cudaStreamSynchronize (warmup)");
                !r) {
                return r;
            }
            start_ = Clock::now();
        }
        return {};
    }

    void startNow() noexcept { start_ = Clock::now(); }
    [[nodiscard]] bool timing() const noexcept { return start_.has_value(); }
    [[nodiscard]] Clock::time_point start() const noexcept { return *start_; }
    [[nodiscard]] std::uint64_t count() const noexcept { return count_; }
    [[nodiscard]] pipeline::PixelFormat format() const noexcept { return format_; }
    [[nodiscard]] pipeline::Size size() const noexcept { return size_; }

private:
    std::uint64_t warmup_;
    cudaStream_t stream_;
    std::uint64_t count_ = 0;
    std::optional<Clock::time_point> start_;
    pipeline::PixelFormat format_ = pipeline::PixelFormat::Nv12;
    pipeline::Size size_;
};

struct PassResult {
    std::uint64_t frames = 0;       // images decodees, chauffe comprise
    std::uint64_t frames_timed = 0; // apres la chauffe
    double wall_s = 0.0;
    double demux_ms = 0.0;
    double decode_ms = 0.0;
    pipeline::DecoderPath path = pipeline::DecoderPath::Software;
    std::uint64_t corrupt_packets = 0; // paquets corrompus (demuxeur) ou rejetes (decodeur)
    std::uint64_t frames_expected = 0; // annonce du conteneur (nb_frames), 0 = inconnu
};

double msSince(Clock::time_point t0, Clock::time_point t1) {
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

/// Une passe complete : ouverture, decodage, fermeture (les objets sont detruits au retour).
Result<PassResult> runPass(const BenchArgs& args, BenchReport& report, bool first_pass) {
    auto demux = pipeline::ffmpeg::FfmpegDemuxer::open(args.input);
    if (!demux) {
        return enh::Unexpected<pipeline::PipelineError>(std::move(demux.error()));
    }
    auto store = pipeline::gpu::CudaFrameStore::create();
    if (!store) {
        return enh::Unexpected<pipeline::PipelineError>(std::move(store.error()));
    }
    pipeline::ffmpeg::FfmpegDecoderOptions opts;
    opts.allow_hardware = args.decoder != DecoderMode::Software;
    auto dec = pipeline::gpu::createNvdecDecoder(**demux, *store, opts);
    if (!dec) {
        return enh::Unexpected<pipeline::PipelineError>(std::move(dec.error()));
    }
    const pipeline::VideoStreamInfo& vs = (*demux)->videoStream();
    if (args.decoder == DecoderMode::Nvdec && !(*dec)->choice().try_hardware) {
        return fail(PipelineErrorCode::UnsupportedFormat,
                    "--decoder nvdec: NVDEC not available for this stream (codec=" + vs.codec_name +
                        ", reason=" + std::string((*dec)->choice().reason) + ")");
    }
    if (first_pass) {
        report.input.codec = vs.codec_name;
        report.input.width = vs.coded_size_px.width;
        report.input.height = vs.coded_size_px.height;
    }

    auto* stream = static_cast<cudaStream_t>((*store)->stream());
    TimingSink sink(args.warmup, stream);
    if (args.warmup == 0) {
        sink.startNow();
    }
    PassResult out;
    const std::stop_token no_stop;
    const auto limitReached = [&] {
        return args.max_frames != 0 && sink.count() >= args.max_frames;
    };
    while (!limitReached()) {
        const bool timed = sink.timing();
        const auto t0 = Clock::now();
        auto pkt = (*demux)->read();
        const auto t1 = Clock::now();
        if (!pkt) {
            return enh::Unexpected<pipeline::PipelineError>(std::move(pkt.error()));
        }
        if (!pkt->has_value()) {
            break; // fin du fichier
        }
        if (auto r = (*dec)->decode(&**pkt, sink, no_stop); !r) {
            return enh::Unexpected<pipeline::PipelineError>(std::move(r.error()));
        }
        if (timed) {
            out.demux_ms += msSince(t0, t1);
            out.decode_ms += msSince(t1, Clock::now());
        }
    }
    if (!limitReached()) {
        const bool timed = sink.timing();
        const auto t0 = Clock::now();
        if (auto r = (*dec)->decode(nullptr, sink, no_stop); !r) { // vidange
            return enh::Unexpected<pipeline::PipelineError>(std::move(r.error()));
        }
        if (timed) {
            out.decode_ms += msSince(t0, Clock::now());
        }
    }
    // Fin : une synchronisation (les copies NVDEC -> pool sont asynchrones sur S_dec).
    if (auto r = pipeline::gpu::checkCuda(cudaStreamSynchronize(stream), "cudaStreamSynchronize");
        !r) {
        return enh::Unexpected<pipeline::PipelineError>(std::move(r.error()));
    }
    const auto end = Clock::now();
    out.frames = sink.count();
    out.path = (*dec)->path();
    out.corrupt_packets = (*dec)->corruptPackets();
    out.frames_expected =
        vs.frame_count_estimate > 0 ? static_cast<std::uint64_t>(vs.frame_count_estimate) : 0;
    if (sink.timing()) {
        out.frames_timed = out.frames - (args.warmup == 0 ? 0 : args.warmup);
        out.wall_s = std::chrono::duration<double>(end - sink.start()).count();
    }
    if (first_pass) {
        report.input.pix_fmt = sink.format() == pipeline::PixelFormat::P010 ? "p010" : "nv12";
    }
    if (args.decoder == DecoderMode::Nvdec && out.path != pipeline::DecoderPath::Nvdec) {
        return fail(PipelineErrorCode::UnsupportedFormat,
                    "--decoder nvdec: decoder fell back to software during the pass");
    }
    if (out.frames_timed == 0) {
        return fail(PipelineErrorCode::InvalidParams,
                    "no timed frame: --warmup " + std::to_string(args.warmup) +
                        " >= " + std::to_string(out.frames) + " decoded frames");
    }
    return out;
}

} // namespace

Result<void> runDecodeStage(const BenchArgs& args, DxgiCudaProbe& probe, BenchReport& report) {
    std::error_code ec;
    const std::uintmax_t size =
        std::filesystem::file_size(std::filesystem::path(pipeline::toExtendedPath(args.input)), ec);
    report.input.size_bytes = ec ? 0 : static_cast<std::uint64_t>(size); // absent : 0, pas -1
    double demux_ms = 0.0;
    double decode_ms = 0.0;
    for (std::uint32_t rep = 0; rep < args.repeat; ++rep) {
        if (auto r = pipeline::gpu::checkCuda(cudaDeviceSynchronize(), "cudaDeviceSynchronize");
            !r) {
            return r;
        }
        const VramSample rest = probe.sample().value_or(VramSample{});
        if (rep == 0) {
            report.vram_rest_mib = rest.process_mib;
            report.vram_device_used_rest_mib = rest.device_used_mib;
        }
        Result<PassResult> pass = fail(PipelineErrorCode::Internal, "not run");
        VramPeaks peaks;
        {
            VramMonitor monitor(probe, std::chrono::milliseconds(args.sample_ms));
            pass = runPass(args, report, rep == 0); // objets de la passe detruits au retour
            peaks = monitor.stop();
        }
        report.vram_peak_mib = std::max(report.vram_peak_mib, peaks.process_peak_mib);
        report.vram_device_used_peak_mib =
            std::max(report.vram_device_used_peak_mib, peaks.device_used_peak_mib);
        report.vram_samples += peaks.samples;
        if (!pass) {
            return enh::Unexpected<pipeline::PipelineError>(std::move(pass.error()));
        }
        (void)cudaDeviceSynchronize();
        const double after = probe.processMib();

        report.decoder_path = pass->path == pipeline::DecoderPath::Nvdec ? "nvdec" : "software";
        report.frames_per_pass = pass->frames;
        report.frames_in += pass->frames;
        report.frames_warmup += pass->frames - pass->frames_timed;
        report.frames_out += pass->frames_timed;
        report.wall_s += pass->wall_s;
        demux_ms += pass->demux_ms;
        decode_ms += pass->decode_ms;
        report.repeats.push_back({.frames_out = pass->frames_timed,
                                  .wall_s = pass->wall_s,
                                  .vram_peak_mib = peaks.process_peak_mib,
                                  .vram_after_mib = after});
        report.corrupt_packets += pass->corrupt_packets;
        report.frames_expected = pass->frames_expected;

        // Decodage partiel (ENH-047, QA cycle 2) : un banc ne rapporte pas comme valide le debit
        // d'un fichier abime. Signes : paquet corrompu / rejete, ou moins d'images que l'annonce
        // du conteneur (fichier tronque) quand tout le fichier devait etre decode.
        const bool truncated = args.max_frames == 0 && pass->frames_expected > 0 &&
                               pass->frames < pass->frames_expected;
        if (pass->corrupt_packets > 0 || truncated) {
            const std::string what =
                "partial decode: " + std::to_string(pass->corrupt_packets) +
                " corrupt or rejected packet(s), " + std::to_string(pass->frames) + " frame(s)" +
                (pass->frames_expected > 0
                     ? " of " + std::to_string(pass->frames_expected) + " announced"
                     : std::string{});
            if (!args.allow_corrupt) {
                report.stages = {
                    {.name = "demux", .ms_total = demux_ms, .frames = report.frames_out},
                    {.name = "decode", .ms_total = decode_ms, .frames = report.frames_out}};
                return fail(PipelineErrorCode::SourceCorrupt,
                            what + " (measure refused; --allow-corrupt to measure anyway)");
            }
            if (rep == 0) {
                report.warnings.push_back(what + ": measured anyway (--allow-corrupt)");
            }
        }
    }
    report.stages = {{.name = "demux", .ms_total = demux_ms, .frames = report.frames_out},
                     {.name = "decode", .ms_total = decode_ms, .frames = report.frames_out}};
    if (report.decoder_path == "software") {
        report.warnings.emplace_back("software decode: 'decode' includes the CPU conversion and "
                                     "the upload to GPU memory");
    }
    return {};
}

} // namespace enh::bench::gpu
