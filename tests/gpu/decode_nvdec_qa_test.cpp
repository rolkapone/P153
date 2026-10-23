// QA ENH-008 (qa-tester) : tests complementaires sur la RTX, hors du jeu du gpu-engineer.
//  - repli FFV1 : message DecoderFallback present au journal ;
//  - annulation en cours de decodage (std::stop_token) : retour propre, VRAM revenue au repos
//    (+/- 50 Mio) apres 10 annulations, puis un fichier complet decode correctement.
// Clips : tests/clips (ENH_CLIPS_DIR pour un autre dossier). Absent = ECHEC.
#include "enh/pipeline/core/decode_loop.hpp"
#include "enh/pipeline/core/log.hpp"
#include "enh/pipeline/ffmpeg/ffmpeg_demuxer.hpp"
#include "enh/pipeline/gpu/cuda_frame_store.hpp"
#include "enh/pipeline/gpu/nvdec_decoder.hpp"

#include <cuda_runtime_api.h>
#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <stop_token>
#include <string>
#include <thread>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
// clang-format off
#include <windows.h>
// clang-format on

namespace enh::pipeline {
namespace {

namespace fs = std::filesystem;

fs::path clips() {
    std::array<wchar_t, 4096> buf{};
    const DWORD n = GetEnvironmentVariableW(L"ENH_CLIPS_DIR", buf.data(), static_cast<DWORD>(buf.size()));
    return n > 0 && n < buf.size() ? fs::path(std::wstring(buf.data(), n)) : fs::path(ENH_CLIPS_DIR);
}

std::uint32_t usedMib() {
    std::size_t free_b = 0;
    std::size_t total_b = 0;
    cudaMemGetInfo(&free_b, &total_b);
    return static_cast<std::uint32_t>((total_b - free_b) / (std::size_t{1024} * 1024));
}

/// Compte les images ; demande l'arret apres `stop_after` images (0 = jamais).
class CountingSink final : public IFrameSink {
public:
    CountingSink(std::uint64_t stop_after, std::stop_source& src) : stop_after_(stop_after), src_(src) {}
    Result<void> push(VideoFrame&& /*frame*/, std::stop_token /*st*/) override {
        ++count;
        if (stop_after_ != 0 && count == stop_after_) {
            src_.request_stop();
        }
        return {};
    }
    std::uint64_t count = 0;

private:
    std::uint64_t stop_after_;
    std::stop_source& src_;
};

struct RunOut {
    Result<DecodeStats> stats = fail(PipelineErrorCode::Internal, "not run");
    std::uint64_t frames_seen = 0;
};

RunOut run(const fs::path& p, std::uint64_t stop_after) {
    RunOut out;
    std::stop_source src;
    CountingSink sink(stop_after, src);
    auto demux = ffmpeg::FfmpegDemuxer::open(p);
    if (!demux) {
        out.stats = enh::Unexpected<PipelineError>(demux.error());
        return out;
    }
    auto dec = gpu::createNvdecDecoder(**demux, gpu::NvdecDecoderOptions{});
    if (!dec) {
        out.stats = enh::Unexpected<PipelineError>(dec.error());
        return out;
    }
    out.stats = runDecodeLoop(**demux, **dec, sink, src.get_token());
    out.frames_seen = sink.count;
    return out;
}

class ENH008Qa : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        auto ctx = gpu::initPrimaryContext(0);
        ASSERT_TRUE(ctx) << ctx.error().detail;
    }
};

TEST_F(ENH008Qa, RepliFfv1_MessageAuJournal) {
    std::mutex m;
    std::vector<std::string> msgs;
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    setLogSink([&](LogLevel level, std::string_view code, std::string_view msg) {
        if (level == LogLevel::Warn && code == "DecoderFallback") {
            const std::scoped_lock lock(m);
            msgs.emplace_back(msg);
        }
    });
    auto r = run(clips() / "in720" / "BBB-FAST.mkv", 0);
    setLogSink({});
    ASSERT_TRUE(r.stats);
    EXPECT_EQ(r.stats->path, DecoderPath::Software);
    EXPECT_EQ(r.frames_seen, 300U);
    ASSERT_EQ(msgs.size(), 1U);
    std::cout << "[QA] message de repli FFV1 : " << msgs[0] << "\n";
    EXPECT_NE(msgs[0].find("codec=ffv1"), std::string::npos);
}

TEST_F(ENH008Qa, Annulation_DixFois_VramRevenueAuRepos) {
    const fs::path p = clips() / "in720" / "BBB-PAN.mp4";
    // chauffe : un decodage complet (initialisations paresseuses)
    ASSERT_TRUE(run(p, 0).stats);
    cudaDeviceSynchronize();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    const std::uint32_t rest = usedMib();
    for (int i = 0; i < 10; ++i) {
        const auto t0 = std::chrono::steady_clock::now();
        auto r = run(p, 40);
        const auto dt = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        // Annule : soit un resultat partiel, soit une erreur Cancelled ; jamais plus d'images que demande + file.
        std::cout << "[QA] annulation " << i << " : " << (r.stats ? "stats" : "erreur") << ", " << r.frames_seen
                  << " images vues, " << dt << " ms\n";
        EXPECT_LT(r.frames_seen, 300U);
        EXPECT_LE(r.frames_seen, 40U + 8U);
    }
    cudaDeviceSynchronize();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    const std::uint32_t after = usedMib();
    std::cout << "[QA] VRAM repos " << rest << " Mio, apres 10 annulations " << after << " Mio\n";
    EXPECT_LE(static_cast<long>(after) - static_cast<long>(rest), 50);
    auto full = run(p, 0);
    ASSERT_TRUE(full.stats);
    EXPECT_EQ(full.frames_seen, 300U);
}

} // namespace
} // namespace enh::pipeline
