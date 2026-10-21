// Demux + decodage LOGICIEL (CI sans GPU, FFmpeg present) : nombre d'images, formats de sortie,
// AVIO maison (Unicode, > 260 caracteres), ouvertures secondaires refusees, fichiers abimes,
// bornes SEC-13, repli journalise, erreur propre d'un magasin d'images defaillant.
#include "enh/pipeline/core/decode_loop.hpp"
#include "enh/pipeline/core/log.hpp"
#include "enh/pipeline/core/win32_file.hpp"
#include "enh/pipeline/ffmpeg/ffmpeg_decoder.hpp"
#include "enh/pipeline/ffmpeg/ffmpeg_demuxer.hpp"

#include "../fakes/fake_stages.hpp"
#include "clip_writer.hpp"

#include <gtest/gtest.h>

#include <fstream>
#include <mutex>
#include <string>
#include <vector>

extern "C" {
#include <libavutil/hwcontext.h>
}

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace enh::pipeline {
namespace {

namespace fs = std::filesystem;
using ffmpeg::FfmpegDecoder;
using ffmpeg::FfmpegDemuxer;
using ffmpeg::HostFrameStore;
using testing::ClipSpec;
using testing::ScopedTempDir;
using testing::writeClip;

/// Capture du journal du pipeline pendant la vie de l'objet.
class LogCapture {
public:
    LogCapture() {
        setLogSink([this](LogLevel level, std::string_view code, std::string_view msg) {
            const std::scoped_lock lock(mutex_);
            lines_.push_back({level, std::string(code), std::string(msg)});
        });
    }
    LogCapture(const LogCapture&) = delete;
    LogCapture& operator=(const LogCapture&) = delete;
    ~LogCapture() { setLogSink({}); }
    struct Line {
        LogLevel level;
        std::string code;
        std::string message;
    };
    std::vector<Line> lines() {
        const std::scoped_lock lock(mutex_);
        return lines_;
    }
    bool has(std::string_view code, LogLevel level) {
        for (const auto& l : lines()) {
            if (l.code == code && l.level == level) {
                return true;
            }
        }
        return false;
    }

private:
    std::mutex mutex_;
    std::vector<Line> lines_;
};

struct DecodeOutcome {
    Result<DecodeStats> stats = fail(PipelineErrorCode::Internal, "not run");
    fakes::CollectingSink sink;
};

std::unique_ptr<DecodeOutcome> decodeFile(const fs::path& p,
                                          std::shared_ptr<ffmpeg::IFrameStore> store = nullptr,
                                          const ffmpeg::FfmpegDecoderOptions& opts = {}) {
    auto out = std::make_unique<DecodeOutcome>();
    auto demux = FfmpegDemuxer::open(p);
    if (!demux) {
        out->stats = enh::Unexpected<PipelineError>(demux.error());
        return out;
    }
    if (!store) {
        store = std::make_shared<HostFrameStore>();
    }
    auto dec = FfmpegDecoder::create((*demux)->codecParameters(), (*demux)->streamTimeBase(),
                                     (*demux)->videoStream(), store, opts);
    if (!dec) {
        out->stats = enh::Unexpected<PipelineError>(dec.error());
        return out;
    }
    out->stats = runDecodeLoop(**demux, **dec, out->sink, {});
    return out;
}

class FfmpegDecode : public ::testing::Test {
protected:
    /// Critere ENH-008 : nombre d images = ffprobe -count_frames de la meme build FFmpeg.
    /// (Les clips MP4 / MOV generes ici perdent une image a l ecriture : ffprobe aussi.)
    static void expectFrameCount(const fs::path& p, std::uint64_t decoded, int written) {
#ifdef ENH_FFPROBE_EXE
        const auto ref = testing::ffprobeFrameCount(ENH_FFPROBE_EXE, p);
        ASSERT_TRUE(ref.has_value()) << "ffprobe a echoue sur " << p.string();
        EXPECT_EQ(decoded, static_cast<std::uint64_t>(*ref)) << p.filename().string();
#endif
        EXPECT_GE(decoded + 1, static_cast<std::uint64_t>(written)) << p.filename().string();
        EXPECT_LE(decoded, static_cast<std::uint64_t>(written)) << p.filename().string();
    }
    fs::path clip(const std::string& name, const ClipSpec& spec) {
        const fs::path p = dir_.path() / name;
        const std::string err = writeClip(p, spec);
        EXPECT_TRUE(err.empty()) << name << ": " << err;
        return p;
    }
    ScopedTempDir dir_;
};

TEST_F(FfmpegDecode, ConteneursEtCodecsLogiciels_NombreDImagesExact) {
    struct Case {
        const char* file;
        ClipSpec spec;
        PixelFormat expected;
    };
    const std::vector<Case> cases = {
        {"a.ts", {"mpegts", "mpeg2video"}, PixelFormat::Nv12},
        {"b.mkv", {"matroska", "ffv1"}, PixelFormat::Nv12},
        {"c.avi", {"avi", "mpeg4"}, PixelFormat::Nv12},
        {"d.mp4", {"mp4", "mpeg4"}, PixelFormat::Nv12},
        {"e.mkv", {"matroska", "mpeg2video", AV_PIX_FMT_YUV422P}, PixelFormat::Nv12},
        {"f.mkv", {"matroska", "ffv1", AV_PIX_FMT_YUV420P10LE}, PixelFormat::P010},
        {"g.mov",
         {"mov", "prores_ks", AV_PIX_FMT_YUV422P10LE, 160, 90, 30, 25, 3},
         PixelFormat::P010},
        {"h.mkv", {"matroska", "ffv1", AV_PIX_FMT_YUV420P, 161, 91}, PixelFormat::Nv12},
    };
    for (const auto& c : cases) {
        const fs::path p = clip(c.file, c.spec);
        auto r = decodeFile(p);
        ASSERT_TRUE(r->stats) << c.file << ": " << r->stats.error().detail;
        expectFrameCount(p, r->stats->frames, c.spec.frames);
        EXPECT_EQ(r->stats->path, DecoderPath::Software) << c.file;
        ASSERT_FALSE(r->sink.formats.empty());
        EXPECT_EQ(r->sink.formats.front(), c.expected) << c.file;
        EXPECT_EQ(r->sink.memory.front(), MemoryKind::Host) << c.file;
        for (std::size_t i = 1; i < r->sink.pts.size(); ++i) {
            if (r->sink.pts[i] == kNoPts) {
                continue; // AVI : pas d horodatage de presentation pour la derniere image
            }
            EXPECT_LT(r->sink.pts[i - 1], r->sink.pts[i]) << c.file << " image " << i;
        }
    }
}

TEST_F(FfmpegDecode, InfosDeLaPiste) {
    const fs::path p = clip("i.mkv", {"matroska", "mpeg2video", AV_PIX_FMT_YUV422P, 320, 180});
    auto demux = FfmpegDemuxer::open(p);
    ASSERT_TRUE(demux) << demux.error().detail;
    const VideoStreamInfo& s = (*demux)->videoStream();
    EXPECT_EQ(s.codec, SourceCodec::Mpeg2);
    EXPECT_EQ(s.codec_name, "mpeg2video");
    EXPECT_EQ(s.coded_size_px, (Size{320, 180}));
    EXPECT_EQ(s.chroma, ChromaSubsampling::Yuv422);
    EXPECT_EQ(s.bit_depth, 8);
    EXPECT_EQ(s.avg_fps, (Rational{25, 1}));
    EXPECT_NEAR(s.duration.count(), 1.2, 0.1);
}

TEST_F(FfmpegDecode, RechercheTemporelle) {
    const fs::path p = clip("s.mkv", {"matroska", "ffv1", AV_PIX_FMT_YUV420P, 64, 36, 75});
    auto demux = FfmpegDemuxer::open(p);
    ASSERT_TRUE(demux);
    ASSERT_TRUE((*demux)->seek(std::chrono::duration<double>(2.0)));
    auto pkt = (*demux)->read();
    ASSERT_TRUE(pkt && pkt->has_value());
    const double t =
        static_cast<double>((*pkt)->pts) * (*pkt)->time_base.num / (*pkt)->time_base.den;
    EXPECT_NEAR(t, 2.0, 0.5);
    EXPECT_FALSE((*demux)->seek(std::chrono::duration<double>(-1.0)));
}

TEST_F(FfmpegDecode, CheminsUnicodeEtLongs_AucunCheminTexteDonneAFfmpeg) {
    const fs::path src = clip("src.ts", {"mpegts", "mpeg2video"});
    fs::path longdir = dir_.path();
    for (int i = 0; i < 5; ++i) {
        longdir /= std::wstring(55, static_cast<wchar_t>(L'k' + i));
        ASSERT_TRUE(CreateDirectoryW(toExtendedPath(longdir).c_str(), nullptr));
    }
    const std::vector<fs::path> targets = {dir_.path() / L"té 日本語 & espace.ts",
                                           dir_.path() / L"clip \U0001F3AC.ts",
                                           longdir / L"vidéo très longue.ts"};
    for (const auto& t : targets) {
        ASSERT_TRUE(CopyFileW(toExtendedPath(src).c_str(), toExtendedPath(t).c_str(), FALSE));
        auto r = decodeFile(t);
        ASSERT_TRUE(r->stats) << r->stats.error().detail;
        EXPECT_EQ(r->stats->frames, 30U);
    }
    EXPECT_GT(targets.back().native().size(), 260U);
}

TEST_F(FfmpegDecode, OuverturesSecondairesRefusees_Hls) {
    clip("seg.ts", {"mpegts", "mpeg2video"});
    const fs::path m3u8 = dir_.path() / "list.m3u8";
    std::ofstream(m3u8) << "#EXTM3U\n#EXT-X-VERSION:3\n#EXT-X-TARGETDURATION:2\n"
                           "#EXTINF:1.2,\nseg.ts\n#EXT-X-ENDLIST\n";
    LogCapture log;
    auto r = decodeFile(m3u8);
    // Aucun chemin texte n'est donne a FFmpeg : la liste n'est meme pas reconnue ; le segment
    // n'est jamais lu.
    ASSERT_FALSE(r->stats);
    EXPECT_EQ(r->stats.error().code, PipelineErrorCode::UnsupportedFormat);
    for (const auto& l : log.lines()) {
        EXPECT_EQ(l.message.find("seg.ts"), std::string::npos) << "URL journalisee : " << l.message;
    }
}

TEST_F(FfmpegDecode, OuverturesSecondairesRefusees_Concat) {
    clip("part.ts", {"mpegts", "mpeg2video"});
    const fs::path list = dir_.path() / "list.ffconcat";
    std::ofstream(list) << "ffconcat version 1.0\nfile 'part.ts'\nfile 'part.ts'\n";
    // Dossier courant = dossier du segment : sans le refus, concat l'ouvrirait lui-meme.
    const fs::path old_cwd = fs::current_path();
    fs::current_path(dir_.path());
    LogCapture log;
    auto r = decodeFile(list);
    fs::current_path(old_cwd);
    ASSERT_FALSE(r->stats);
    EXPECT_EQ(r->stats.error().code, PipelineErrorCode::UnsupportedFormat);
    EXPECT_NE(r->stats.error().detail.find("concat"), std::string::npos);
    EXPECT_TRUE(log.has("DemuxerRefused", LogLevel::Warn));
}

TEST(FfmpegDemuxerPolicy, DemuxeursRefuses) {
    for (const char* name : {"concat", "hls", "dash", "imf", "image2", "sdp", "rtsp", "rtp"}) {
        EXPECT_TRUE(ffmpeg::isRefusedDemuxer(name)) << name;
    }
    for (const char* name :
         {"mov,mp4,m4a,3gp,3g2,mj2", "matroska,webm", "mpegts", "avi", "image2pipe", "mpeg", ""}) {
        EXPECT_FALSE(ffmpeg::isRefusedDemuxer(name)) << name;
    }
    EXPECT_TRUE(ffmpeg::isRefusedDemuxer("foo,concat"));
}

TEST_F(FfmpegDecode, FichiersAbimes_SansPlantage) {
    const fs::path good =
        clip("good.ts", {"mpegts", "mpeg2video", AV_PIX_FMT_YUV420P, 160, 90, 60});
    const auto size = fs::file_size(good);
    // 0 octet
    const fs::path empty = dir_.path() / "empty.mp4";
    std::ofstream(empty).close();
    auto r0 = decodeFile(empty);
    ASSERT_FALSE(r0->stats);
    EXPECT_EQ(r0->stats.error().code, PipelineErrorCode::UnsupportedFormat);
    // Tronque a 40 %
    const fs::path cut = dir_.path() / "cut.ts";
    fs::copy_file(good, cut);
    fs::resize_file(cut, size * 4 / 10);
    auto r1 = decodeFile(cut);
    if (r1->stats) {
        EXPECT_LT(r1->stats->frames, 60U);
    }
    // Octets ecrases au milieu
    const fs::path bad = dir_.path() / "bad.ts";
    fs::copy_file(good, bad);
    {
        std::fstream f(bad, std::ios::in | std::ios::out | std::ios::binary);
        f.seekp(static_cast<std::streamoff>(size / 2));
        const std::string junk(4096, '\x5A');
        f.write(junk.data(), static_cast<std::streamsize>(junk.size()));
    }
    auto r2 = decodeFile(bad);
    if (r2->stats) {
        EXPECT_LE(r2->stats->frames, 60U);
    }
    // Pas un conteneur video
    const fs::path text = dir_.path() / "notes.mp4";
    std::ofstream(text) << "ceci n'est pas une video";
    auto r3 = decodeFile(text);
    ASSERT_FALSE(r3->stats);
    EXPECT_EQ(r3->stats.error().code, PipelineErrorCode::UnsupportedFormat);
    // Absent
    auto r4 = decodeFile(dir_.path() / "absent.mkv");
    ASSERT_FALSE(r4->stats);
    EXPECT_EQ(r4->stats.error().code, PipelineErrorCode::SourceMissing);
}

TEST_F(FfmpegDecode, DimensionsHorsBornes_RefuseesAvantAllocation) {
    const fs::path p = clip("wide.mkv", {"matroska", "ffv1", AV_PIX_FMT_YUV420P, 8200, 16, 2});
    auto demux = FfmpegDemuxer::open(p);
    ASSERT_FALSE(demux);
    EXPECT_EQ(demux.error().code, PipelineErrorCode::UnsupportedFormat);
    EXPECT_NE(demux.error().detail.find("8200"), std::string::npos);
}

TEST_F(FfmpegDecode, SansGpu_RepliJournaliseEnInfo) {
    const fs::path p = clip("j.ts", {"mpegts", "mpeg2video"});
    LogCapture log;
    auto r = decodeFile(p);
    ASSERT_TRUE(r->stats);
    EXPECT_TRUE(log.has("DecoderFallback", LogLevel::Info));
}

/// Magasin CPU qui annonce un contexte materiel (alloue, jamais initialise ni utilise) : simule
/// une machine avec GPU pour verifier le repli d'un codec non gere par NVDEC.
class FakeHwStore final : public ffmpeg::IFrameStore {
public:
    FakeHwStore() : hw_(av_hwdevice_ctx_alloc(AV_HWDEVICE_TYPE_CUDA)) {}
    [[nodiscard]] AVBufferRef* hwDevice() const noexcept override { return hw_.get(); }
    Result<FrameBufferPtr> acquireStaging(const FrameDesc& d, std::stop_token st) override {
        return host_.acquireStaging(d, std::move(st));
    }
    Result<FrameBufferPtr> commit(FrameBufferPtr s, std::stop_token st) override {
        return host_.commit(std::move(s), std::move(st));
    }
    [[nodiscard]] MemoryKind outputMemory() const noexcept override { return MemoryKind::Host; }

private:
    ffmpeg::BufferRefPtr hw_;
    HostFrameStore host_;
};

TEST_F(FfmpegDecode, ProResAvecGpu_RepliLogicielJournaliseEnAvertissement) {
    const fs::path p =
        clip("pr.mov", {"mov", "prores_ks", AV_PIX_FMT_YUV422P10LE, 160, 90, 20, 25, 3});
    LogCapture log;
    auto r = decodeFile(p, std::make_shared<FakeHwStore>());
    ASSERT_TRUE(r->stats) << r->stats.error().detail;
    expectFrameCount(p, r->stats->frames, 20);
    EXPECT_EQ(r->stats->path, DecoderPath::Software);
    bool found = false;
    for (const auto& l : log.lines()) {
        if (l.code == "DecoderFallback" && l.level == LogLevel::Warn &&
            l.message.find("codec=prores") != std::string::npos &&
            l.message.find("reason=nvdec-unsupported-codec") != std::string::npos) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

/// Magasin qui echoue (pilote absent simule) : l'erreur remonte telle quelle, sans plantage.
class FailingStore final : public ffmpeg::IFrameStore {
public:
    Result<FrameBufferPtr> acquireStaging(const FrameDesc&, std::stop_token) override {
        PipelineError e = makeError(PipelineErrorCode::ComponentMissing, "no driver (simulated)");
        e.component = Component::Driver;
        return enh::Unexpected<PipelineError>(std::move(e));
    }
    Result<FrameBufferPtr> commit(FrameBufferPtr s, std::stop_token) override { return s; }
    [[nodiscard]] MemoryKind outputMemory() const noexcept override { return MemoryKind::Device; }
};

TEST_F(FfmpegDecode, MagasinDefaillant_ErreurPropre) {
    const fs::path p = clip("k.ts", {"mpegts", "mpeg2video"});
    auto r = decodeFile(p, std::make_shared<FailingStore>());
    ASSERT_FALSE(r->stats);
    EXPECT_EQ(r->stats.error().code, PipelineErrorCode::ComponentMissing);
    EXPECT_EQ(r->stats.error().component, Component::Driver);
}

TEST_F(FfmpegDecode, PaquetsSansObjetNatif_CopiesPourLeDecodeur) {
    // Un demuxeur non-FFmpeg (octets bruts) est accepte : le decodeur recopie le paquet.
    const fs::path p = clip("raw.ts", {"mpegts", "mpeg2video"});
    auto demux = FfmpegDemuxer::open(p);
    ASSERT_TRUE(demux);
    auto store = std::make_shared<HostFrameStore>();
    auto dec = FfmpegDecoder::create((*demux)->codecParameters(), (*demux)->streamTimeBase(),
                                     (*demux)->videoStream(), store);
    ASSERT_TRUE(dec);
    fakes::CollectingSink sink;
    for (;;) {
        auto pkt = (*demux)->read();
        ASSERT_TRUE(pkt);
        if (!pkt->has_value()) {
            break;
        }
        const auto bytes = (*pkt)->payload->bytes();
        Packet copy;
        copy.payload = std::make_unique<fakes::BytesPayload>(
            std::vector<std::byte>(bytes.begin(), bytes.end()));
        copy.pts = (*pkt)->pts;
        copy.dts = (*pkt)->dts;
        copy.keyframe = (*pkt)->keyframe;
        ASSERT_TRUE((*dec)->decode(&copy, sink, {}));
    }
    ASSERT_TRUE((*dec)->decode(nullptr, sink, {}));
    EXPECT_EQ(sink.pts.size(), 30U);
}

} // namespace
} // namespace enh::pipeline
