// ENH-008 sur la RTX (local seulement, label gpu) : matrice noyau, nombre d'images = ffprobe,
// chemin NVDEC / logiciel, images en memoire GPU, contexte CUDA unique, exactitude des pixels,
// vitesse >= 1 000 i/s et VRAM <= 600 Mio sur BBB-FAST.mp4, 10 fichiers sans fuite, cas limites.
// Corpus : tests/gpu/corpus/make-corpus.ps1 (absent = ECHEC, jamais de PASS par defaut).
#include "enh/pipeline/core/decode_loop.hpp"
#include "enh/pipeline/core/log.hpp"
#include "enh/pipeline/core/win32_file.hpp"
#include "enh/pipeline/ffmpeg/ffmpeg_demuxer.hpp"
#include "enh/pipeline/gpu/cuda_frame_store.hpp"
#include "enh/pipeline/gpu/nvdec_decoder.hpp"

#include "ffmpeg/clip_writer.hpp"

#include <cuda_runtime_api.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
// clang-format off : <windows.h> doit preceder <psapi.h> et <tlhelp32.h>
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
// clang-format on

namespace enh::pipeline {
namespace {

namespace fs = std::filesystem;
using ffmpeg::FfmpegDecoder;
using ffmpeg::FfmpegDemuxer;
using gpu::CudaFrameStore;

fs::path clipsDir() {
    std::array<wchar_t, 4096> buf{};
    const DWORD n =
        GetEnvironmentVariableW(L"ENH_CLIPS_DIR", buf.data(), static_cast<DWORD>(buf.size()));
    if (n > 0 && n < buf.size()) {
        return fs::path(std::wstring(buf.data(), n));
    }
    return fs::path(ENH_CLIPS_DIR);
}

fs::path requireClip(const fs::path& rel) {
    const fs::path p = clipsDir() / rel;
    EXPECT_TRUE(fs::exists(fs::path(toExtendedPath(p))))
        << "Clip absent : " << p.string()
        << " (lancer tests/gpu/corpus/make-corpus.ps1 ou scripts/fetch-clips.ps1)";
    return p;
}

/// Verifie chaque image : memoire GPU, format attendu, evenement « pret » ; ne garde rien.
class CheckingSink final : public IFrameSink {
public:
    Result<void> push(VideoFrame&& frame, std::stop_token /*st*/) override {
        ++count;
        if (frame.buffer->memory() == MemoryKind::Device) {
            ++on_device;
        }
        if (frame.buffer->readyEvent() == nullptr) {
            ++without_event;
        }
        formats_seen |= (1U << static_cast<unsigned>(frame.desc.format));
        size = frame.desc.size_px;
        return {};
    }
    std::uint64_t count = 0;
    std::uint64_t on_device = 0;
    std::uint64_t without_event = 0;
    unsigned formats_seen = 0;
    Size size;
};

/// Recopie l'image numero `index` en memoire CPU (plan Y) pour comparaison.
class CaptureSink final : public IFrameSink {
public:
    explicit CaptureSink(std::int64_t index) : index_(index) {}
    Result<void> push(VideoFrame&& frame, std::stop_token /*st*/) override {
        if (frame.index != index_) {
            return {};
        }
        const PlaneView y = frame.buffer->planes()[0];
        luma.resize(y.width_bytes * y.rows);
        if (frame.buffer->memory() == MemoryKind::Device) {
            cudaEventSynchronize(static_cast<cudaEvent_t>(frame.buffer->readyEvent()));
            const cudaError_t e =
                cudaMemcpy2D(luma.data(), y.width_bytes, reinterpret_cast<const void*>(y.address),
                             y.pitch_bytes, y.width_bytes, y.rows, cudaMemcpyDeviceToHost);
            EXPECT_EQ(e, cudaSuccess);
        } else {
            for (std::size_t r = 0; r < y.rows; ++r) {
                std::memcpy(luma.data() + r * y.width_bytes,
                            reinterpret_cast<const std::byte*>(y.address) + r * y.pitch_bytes,
                            y.width_bytes);
            }
        }
        return {};
    }
    std::vector<std::byte> luma;

private:
    std::int64_t index_;
};

struct RunResult {
    Result<DecodeStats> stats = fail(PipelineErrorCode::Internal, "not run");
    std::string decoder_name;
};

RunResult decodeWithNvdec(const fs::path& p, IFrameSink& sink,
                          const gpu::NvdecDecoderOptions& opts = {}) {
    RunResult out;
    auto demux = FfmpegDemuxer::open(p);
    if (!demux) {
        out.stats = enh::Unexpected<PipelineError>(demux.error());
        return out;
    }
    auto dec = gpu::createNvdecDecoder(**demux, opts);
    if (!dec) {
        out.stats = enh::Unexpected<PipelineError>(dec.error());
        return out;
    }
    out.decoder_name = std::string((*dec)->decoderName());
    out.stats = runDecodeLoop(**demux, **dec, sink, {});
    return out;
}

std::uint32_t usedMib() {
    std::size_t free_b = 0;
    std::size_t total_b = 0;
    cudaMemGetInfo(&free_b, &total_b);
    return static_cast<std::uint32_t>((total_b - free_b) / (1024 * 1024));
}

/// Echantillonne la VRAM utilisee (cudaMemGetInfo) toutes les 10 ms dans un thread.
class VramSampler {
public:
    VramSampler()
        : thread_([this](std::stop_token st) {
              cudaSetDevice(0);
              while (!st.stop_requested()) {
                  const std::uint32_t u = usedMib();
                  std::uint32_t prev = peak_.load();
                  while (u > prev && !peak_.compare_exchange_weak(prev, u)) {
                  }
                  std::this_thread::sleep_for(std::chrono::milliseconds(10));
              }
          }) {}
    std::uint32_t stop() {
        thread_.request_stop();
        thread_.join();
        return std::max(peak_.load(), usedMib());
    }

private:
    std::atomic<std::uint32_t> peak_{0};
    std::jthread thread_;
};

class ENH008Nvdec : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        auto ctx = gpu::initPrimaryContext(0);
        ASSERT_TRUE(ctx) << ctx.error().detail;
    }
};

struct MatrixCase {
    const char* file;
    DecoderPath path;
    PixelFormat format;
    const char* decoder; // nom FFmpeg attendu
};

class ENH008Matrix : public ENH008Nvdec, public ::testing::WithParamInterface<MatrixCase> {};

TEST_P(ENH008Matrix, ImagesEnMemoireGpu_NombreEgalFfprobe) {
    const MatrixCase& c = GetParam();
    const fs::path p = requireClip(c.file);
    CheckingSink sink;
    auto r = decodeWithNvdec(p, sink);
    ASSERT_TRUE(r.stats) << r.stats.error().detail;
    const auto ref = testing::ffprobeFrameCount(ENH_FFPROBE_EXE, p);
    ASSERT_TRUE(ref.has_value()) << "ffprobe a echoue";
    EXPECT_EQ(sink.count, static_cast<std::uint64_t>(*ref)) << c.file;
    EXPECT_GT(sink.count, 0U);
    EXPECT_EQ(sink.on_device, sink.count) << "toutes les images doivent etre en memoire GPU";
    EXPECT_EQ(sink.without_event, 0U);
    EXPECT_EQ(r.stats->path, c.path) << c.file;
    EXPECT_EQ(sink.formats_seen, 1U << static_cast<unsigned>(c.format)) << c.file;
    EXPECT_EQ(r.decoder_name, c.decoder);
    std::cout << "[ENH-008] " << c.file << " : " << sink.count << " images (ffprobe " << *ref
              << "), " << (r.stats->path == DecoderPath::Nvdec ? "NVDEC" : "logiciel") << ", "
              << sink.size.width << "x" << sink.size.height << ", decodeur " << r.decoder_name
              << "\n";
}

INSTANTIATE_TEST_SUITE_P(
    Noyau, ENH008Matrix,
    ::testing::Values(
        MatrixCase{"gen/core_mp4_h264.mp4", DecoderPath::Nvdec, PixelFormat::Nv12, "h264"},
        MatrixCase{"gen/core_mp4_hevc10.mp4", DecoderPath::Nvdec, PixelFormat::P010, "hevc"},
        MatrixCase{"gen/core_mkv_av1_10.mkv", DecoderPath::Nvdec, PixelFormat::P010, "av1"},
        MatrixCase{"gen/core_webm_vp9.webm", DecoderPath::Nvdec, PixelFormat::Nv12, "vp9"},
        MatrixCase{"gen/core_ts_mpeg2.ts", DecoderPath::Nvdec, PixelFormat::Nv12, "mpeg2video"},
        MatrixCase{"gen/core_avi_h264.avi", DecoderPath::Nvdec, PixelFormat::Nv12, "h264"},
        MatrixCase{"gen/core_mov_h264.mov", DecoderPath::Nvdec, PixelFormat::Nv12, "h264"},
        MatrixCase{"gen/core_mov_prores_hq.mov", DecoderPath::Software, PixelFormat::P010,
                   "prores"},
        MatrixCase{"in720/BBB-FAST.mp4", DecoderPath::Nvdec, PixelFormat::Nv12, "h264"},
        MatrixCase{"in720/BBB-FAST.mkv", DecoderPath::Software, PixelFormat::Nv12, "ffv1"}),
    [](const ::testing::TestParamInfo<MatrixCase>& info) {
        std::string n = fs::path(info.param.file).stem().string();
        if (std::string(info.param.file).starts_with("in720")) {
            n += fs::path(info.param.file).extension() == ".mkv" ? "_ffv1" : "_qa";
        }
        std::replace_if(n.begin(), n.end(), [](char ch) { return !std::isalnum(ch); }, '_');
        return n;
    });

TEST_F(ENH008Nvdec, UnSeulContexteCuda_PartageAvecFfmpeg) {
    auto store = CudaFrameStore::create();
    ASSERT_TRUE(store) << store.error().detail;
    EXPECT_NE((*store)->cudaContext(), nullptr);
    EXPECT_EQ((*store)->cudaContext(), (*store)->ffmpegCudaContext());
    auto again = gpu::initPrimaryContext(0);
    ASSERT_TRUE(again);
    EXPECT_EQ(*again, (*store)->cudaContext()) << "contexte primaire attendu";
}

TEST_F(ENH008Nvdec, ModulesPiloteChargesDepuisSystem32) {
    ASSERT_TRUE(gpu::preloadNvidiaDriverDlls());
    std::array<wchar_t, MAX_PATH> sysdir{};
    GetSystemDirectoryW(sysdir.data(), MAX_PATH);
    for (const wchar_t* dll : {L"nvcuda.dll", L"nvcuvid.dll"}) {
        HMODULE m = GetModuleHandleW(dll);
        ASSERT_NE(m, nullptr);
        std::array<wchar_t, MAX_PATH> path{};
        GetModuleFileNameW(m, path.data(), MAX_PATH);
        EXPECT_EQ(_wcsnicmp(path.data(), sysdir.data(), wcslen(sysdir.data())), 0)
            << fs::path(path.data()).string();
    }
}

TEST_F(ENH008Nvdec, PixelsNvdecIdentiquesAuDecodageLogiciel) {
    const fs::path p = requireClip("in720/BBB-FAST.mp4");
    CaptureSink gpu_sink(150);
    auto r = decodeWithNvdec(p, gpu_sink);
    ASSERT_TRUE(r.stats);
    ASSERT_EQ(r.stats->path, DecoderPath::Nvdec);
    gpu::NvdecDecoderOptions sw;
    sw.decoder.allow_hardware = false;
    CaptureSink cpu_sink(150);
    auto s = decodeWithNvdec(p, cpu_sink, sw);
    ASSERT_TRUE(s.stats);
    ASSERT_EQ(s.stats->path, DecoderPath::Software);
    ASSERT_FALSE(gpu_sink.luma.empty());
    ASSERT_EQ(gpu_sink.luma.size(), cpu_sink.luma.size());
    std::size_t diff = 0;
    for (std::size_t i = 0; i < gpu_sink.luma.size(); ++i) {
        diff += gpu_sink.luma[i] != cpu_sink.luma[i] ? 1U : 0U;
    }
    EXPECT_EQ(diff, 0U) << "H.264 est bit-exact : NVDEC et logiciel doivent donner la meme image";
}

TEST_F(ENH008Nvdec, RepliProRes_JournaliseEtEnvoyeSurGpu) {
    const fs::path p = requireClip("gen/core_mov_prores_hq.mov");
    std::mutex m;
    std::vector<std::string> warn;
    setLogSink([&](LogLevel level, std::string_view code, std::string_view msg) {
        if (level == LogLevel::Warn && code == "DecoderFallback") {
            const std::scoped_lock lock(m);
            warn.emplace_back(msg);
        }
    });
    CheckingSink sink;
    auto r = decodeWithNvdec(p, sink);
    setLogSink({});
    ASSERT_TRUE(r.stats);
    EXPECT_EQ(r.stats->path, DecoderPath::Software);
    EXPECT_EQ(sink.on_device, sink.count);
    ASSERT_EQ(warn.size(), 1U);
    EXPECT_NE(warn[0].find("codec=prores"), std::string::npos);
    EXPECT_NE(warn[0].find("reason=nvdec-unsupported-codec"), std::string::npos);
}

TEST_F(ENH008Nvdec, Vitesse_Et_Vram_BbbFast) {
    const fs::path p = requireClip("in720/BBB-FAST.mp4");
    cudaDeviceSynchronize();
    const std::uint32_t rest = usedMib(); // contexte CUDA deja cree (exclu du budget)
    std::vector<double> fps;
    std::uint32_t peak = 0;
    for (int pass = 0; pass < 6; ++pass) { // 1 chauffe + 5 mesures (protocole QA §4)
        VramSampler sampler;
        CheckingSink sink;
        auto r = decodeWithNvdec(p, sink);
        peak = std::max(peak, sampler.stop());
        ASSERT_TRUE(r.stats) << r.stats.error().detail;
        ASSERT_EQ(r.stats->path, DecoderPath::Nvdec);
        if (pass > 0) {
            fps.push_back(static_cast<double>(sink.count) / r.stats->elapsed.count());
        }
    }
    std::sort(fps.begin(), fps.end());
    const double median = fps[fps.size() / 2];
    const double spread = (fps.back() - fps.front()) / median;
    const std::uint32_t stage_mib = peak - rest;
    std::cout << "[ENH-008] BBB-FAST.mp4 NVDEC : mediane " << median << " i/s (min " << fps.front()
              << ", max " << fps.back() << ", ecart " << spread * 100.0 << " %), VRAM etage "
              << stage_mib << " Mio (pic " << peak << ", repos " << rest << ")\n";
    RecordProperty("fps_median", std::to_string(median));
    RecordProperty("vram_stage_mib", std::to_string(stage_mib));
    EXPECT_GE(median, 1000.0);
    EXPECT_LE(stage_mib, 600U);
}

std::size_t privateBytes() {
    PROCESS_MEMORY_COUNTERS_EX pmc{};
    GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc),
                         sizeof(pmc));
    return pmc.PrivateUsage;
}

DWORD handleCount() {
    DWORD n = 0;
    GetProcessHandleCount(GetCurrentProcess(), &n);
    return n;
}

DWORD threadCount() {
    DWORD n = 0;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snap == INVALID_HANDLE_VALUE) {
        return 0;
    }
    THREADENTRY32 te{};
    te.dwSize = sizeof(te);
    const DWORD pid = GetCurrentProcessId();
    for (BOOL ok = Thread32First(snap, &te); ok != FALSE; ok = Thread32Next(snap, &te)) {
        n += te.th32OwnerProcessID == pid ? 1U : 0U;
    }
    CloseHandle(snap);
    return n;
}

TEST_F(ENH008Nvdec, DixFichiersALaSuite_SansFuite) {
    // 10 fichiers DISTINCTS (protocole QA §4.3). Tour 1 = chauffe (initialisations paresseuses
    // de FFmpeg et du pilote, une fois par processus) ; tour 2 = mesure. Une fuite reelle grandit
    // a chaque fichier : derive VRAM du tour 2 et ecart de handles / threads entre les deux tours.
    const std::vector<const char*> files = {"in720/BBB-FAST.mp4",         "gen/core_mp4_h264.mp4",
                                            "gen/core_mp4_hevc10.mp4",    "gen/core_mkv_av1_10.mkv",
                                            "gen/core_webm_vp9.webm",     "gen/core_ts_mpeg2.ts",
                                            "gen/core_avi_h264.avi",      "gen/core_mov_h264.mov",
                                            "gen/core_mov_prores_hq.mov", "in720/BBB-PAN.mp4"};
    struct Point {
        std::uint32_t vram_mib;
        std::size_t private_bytes;
        DWORD handles;
        DWORD threads;
    };
    std::vector<std::vector<Point>> rounds(2);
    for (auto& round : rounds) {
        for (const char* f : files) {
            CheckingSink sink;
            auto r = decodeWithNvdec(requireClip(f), sink);
            ASSERT_TRUE(r.stats) << f << ": " << r.stats.error().detail;
            cudaDeviceSynchronize();
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            round.push_back({usedMib(), privateBytes(), handleCount(), threadCount()});
        }
    }
    const auto& m = rounds[1];
    const long drift_mib =
        static_cast<long>(m.back().vram_mib) - static_cast<long>(m.front().vram_mib);
    const double priv_mib = (static_cast<double>(m.back().private_bytes) -
                             static_cast<double>(m.front().private_bytes)) /
                            (1024.0 * 1024.0);
    const long handle_drift =
        static_cast<long>(m.back().handles) - static_cast<long>(rounds[0].back().handles);
    const long thread_drift =
        static_cast<long>(m.back().threads) - static_cast<long>(rounds[0].back().threads);
    std::cout << "[ENH-008] 10 fichiers (tour mesure) : VRAM";
    for (const auto& p : m) {
        std::cout << ' ' << p.vram_mib;
    }
    std::cout << " Mio ; derive " << drift_mib << " Mio ; memoire privee " << priv_mib
              << " Mio ; handles " << rounds[0].back().handles << " -> " << m.back().handles
              << " ; threads " << rounds[0].back().threads << " -> " << m.back().threads << "\n";
    EXPECT_LE(drift_mib, 50);
    EXPECT_LE(priv_mib, 100.0);
    EXPECT_LE(handle_drift, 0) << "handles en fuite";
    EXPECT_LE(thread_drift, 0) << "threads en fuite";
}

TEST_F(ENH008Nvdec, CasLimites_SansPlantage_VramLiberee) {
    cudaDeviceSynchronize();
    const std::uint32_t before = usedMib();
    {
        CheckingSink sink;
        auto r = decodeWithNvdec(requireClip("gen/edge_zero.mp4"), sink);
        ASSERT_FALSE(r.stats);
        EXPECT_EQ(r.stats.error().code, PipelineErrorCode::UnsupportedFormat);
    }
    {
        CheckingSink sink;
        auto r = decodeWithNvdec(requireClip("gen/edge_truncated.mp4"), sink);
        std::cout << "[ENH-008] tronque : "
                  << (r.stats ? std::to_string(sink.count) + " images"
                              : std::string(toString(r.stats.error().code)))
                  << "\n";
        if (r.stats) {
            EXPECT_LT(sink.count, 300U);
        }
    }
    {
        CheckingSink sink;
        auto r = decodeWithNvdec(requireClip("gen/edge_corrupt.mp4"), sink);
        std::cout << "[ENH-008] corrompu : "
                  << (r.stats ? std::to_string(sink.count) + " images, " +
                                    std::to_string(r.stats->corrupt_packets) + " paquets abimes"
                              : std::string(toString(r.stats.error().code)))
                  << "\n";
    }
    {
        CheckingSink sink;
        const std::wstring uni = std::wstring(L"té 日本語 & espace.mkv");
        auto r = decodeWithNvdec(requireClip(fs::path(L"gen") / uni), sink);
        ASSERT_TRUE(r.stats) << r.stats.error().detail;
        EXPECT_EQ(r.stats->path, DecoderPath::Nvdec);
        EXPECT_GT(sink.count, 0U);
    }
    {
        fs::path longp = clipsDir() / L"gen" / L"long";
        for (int i = 0; i < 5; ++i) {
            longp /= std::wstring(55, static_cast<wchar_t>(L'a' + i));
        }
        longp /= L"vidéo longue.mp4";
        ASSERT_GT(longp.native().size(), 260U);
        CheckingSink sink;
        auto r = decodeWithNvdec(longp, sink);
        ASSERT_TRUE(r.stats) << r.stats.error().detail;
        EXPECT_EQ(sink.count, 300U);
    }
    cudaDeviceSynchronize();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    const long delta = static_cast<long>(usedMib()) - static_cast<long>(before);
    EXPECT_LE(std::abs(delta), 50) << "VRAM non liberee apres les cas limites";
}

} // namespace
} // namespace enh::pipeline
