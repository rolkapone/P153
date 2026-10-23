// Boucle demux -> decodage sur faux : ordre, vidange, erreur injectee, annulation, back-pressure.
#include "enh/pipeline/core/decode_loop.hpp"
#include "enh/pipeline/core/host_frame_pool.hpp"
#include "enh/pipeline/core/log.hpp"

#include "fakes/fake_stages.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <thread>

namespace enh::pipeline {
namespace {

using fakes::CollectingSink;
using fakes::FakeDecoder;
using fakes::FakeDemuxer;

TEST(DecodeLoop, ToutesLesImagesDansLOrdre_AvecVidange) {
    FakeDemuxer demux(25);
    FakeDecoder dec(3);
    CollectingSink sink;
    auto stats = runDecodeLoop(demux, dec, sink, {});
    ASSERT_TRUE(stats) << stats.error().detail;
    EXPECT_EQ(stats->packets, 25U);
    EXPECT_EQ(stats->frames, 25U);
    ASSERT_EQ(sink.pts.size(), 25U);
    for (std::size_t i = 0; i < sink.pts.size(); ++i) {
        EXPECT_EQ(sink.pts[i], static_cast<std::int64_t>(i));
    }
    EXPECT_EQ(dec.pool().inUse(), 0U); // aucune image retenue apres la boucle
}

TEST(DecodeLoop, FichierVide) {
    FakeDemuxer demux(0);
    FakeDecoder dec;
    CollectingSink sink;
    auto stats = runDecodeLoop(demux, dec, sink, {});
    ASSERT_TRUE(stats);
    EXPECT_EQ(stats->frames, 0U);
}

TEST(DecodeLoop, ErreurInjecteeRendueTelleQuelle) {
    FakeDemuxer demux(50, 17, PipelineErrorCode::SourceMissing);
    FakeDecoder dec(2);
    CollectingSink sink;
    auto stats = runDecodeLoop(demux, dec, sink, {});
    ASSERT_FALSE(stats);
    EXPECT_EQ(stats.error().code, PipelineErrorCode::SourceMissing);
    EXPECT_EQ(sink.pts.size(), 15U); // 17 paquets lus, 2 retenus par le decodeur
}

TEST(DecodeLoop, AnnulationParStopToken) {
    FakeDemuxer demux(1000);
    FakeDecoder dec(0);
    std::stop_source stop;
    CollectingSink sink(&stop, 10);
    auto stats = runDecodeLoop(demux, dec, sink, stop.get_token());
    ASSERT_FALSE(stats);
    EXPECT_EQ(stats.error().code, PipelineErrorCode::Cancelled);
    EXPECT_EQ(sink.pts.size(), 10U);
}

TEST(DecodeLoop, LimiteDImages) {
    FakeDemuxer demux(100);
    FakeDecoder dec(0);
    CollectingSink sink;
    auto stats = runDecodeLoop(demux, dec, sink, {}, 30);
    ASSERT_TRUE(stats);
    EXPECT_EQ(stats->frames, 30U);
}

TEST(HostFramePool, BorneEtRetourAuPool) {
    auto pool = HostFramePool::create(FrameDesc{Size{17, 9}, PixelFormat::P010, {}}, 2);
    ASSERT_TRUE(pool);
    auto a = (*pool)->acquire({});
    auto b = (*pool)->acquire({});
    ASSERT_TRUE(a && b);
    EXPECT_EQ((*pool)->inUse(), 2U);
    const auto planes = (*a)->planes();
    EXPECT_EQ(planes[0].address % HostFramePool::kPitchAlignment, 0U);
    EXPECT_EQ(planes[0].width_bytes, 34U);
    EXPECT_EQ(planes[1].width_bytes, 36U);
    // Pool plein : acquire bloque, et l'arret du stop_token le debloque (Cancelled).
    std::stop_source stop;
    std::jthread canceller([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        stop.request_stop();
    });
    auto c = (*pool)->acquire(stop.get_token());
    ASSERT_FALSE(c);
    EXPECT_EQ(c.error().code, PipelineErrorCode::Cancelled);
    a->reset();
    EXPECT_EQ((*pool)->inUse(), 1U);
    auto d = (*pool)->acquire({});
    EXPECT_TRUE(d);
}

TEST(HostFramePool, ImageSurvitAuPool) {
    FrameBufferPtr survivor;
    {
        auto pool = HostFramePool::create(FrameDesc{Size{8, 8}, PixelFormat::Nv12, {}}, 1);
        ASSERT_TRUE(pool);
        auto img = (*pool)->acquire({});
        ASSERT_TRUE(img);
        survivor = std::move(*img);
    } // le pool n'a plus de proprietaire... sauf l'image
    auto* p = reinterpret_cast<std::byte*>(survivor->planes()[0].address);
    p[0] = std::byte{42}; // memoire toujours valide
    survivor.reset();     // liberation finale sans plantage
}

TEST(HostFramePool, ParametresInvalides) {
    EXPECT_FALSE(HostFramePool::create(FrameDesc{Size{8, 8}, PixelFormat::Nv12, {}}, 0));
    EXPECT_FALSE(HostFramePool::create(FrameDesc{Size{9000, 8}, PixelFormat::Nv12, {}}, 1));
}

TEST(PipelineLog, RecepteurBranche) {
    std::vector<std::string> lines;
    setLogSink([&](LogLevel, std::string_view code, std::string_view msg) {
        lines.emplace_back(std::string(code) + ":" + std::string(msg));
    });
    logLine(LogLevel::Warn, "DecoderFallback", "codec=prores");
    setLogSink({});
    logLine(LogLevel::Info, "Ignored", "vers le debogueur");
    ASSERT_EQ(lines.size(), 1U);
    EXPECT_EQ(lines[0], "DecoderFallback:codec=prores");
}

} // namespace
} // namespace enh::pipeline
