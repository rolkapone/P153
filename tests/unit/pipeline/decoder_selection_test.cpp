// Choix du decodeur : NVDEC tente seulement quand il gere le flux ; AV1 natif pour NVDEC.
#include "enh/pipeline/core/decoder_selection.hpp"

#include <gtest/gtest.h>

namespace enh::pipeline {
namespace {

VideoStreamInfo stream(SourceCodec c, int bits = 8,
                       ChromaSubsampling chroma = ChromaSubsampling::Yuv420) {
    VideoStreamInfo s;
    s.codec = c;
    s.bit_depth = bits;
    s.chroma = chroma;
    return s;
}

TEST(DecoderSelection, MatriceNoyauSurNvdec) {
    struct Case {
        SourceCodec codec;
        int bits;
        std::string_view name;
    };
    for (const Case c : {Case{SourceCodec::H264, 8, "h264"}, Case{SourceCodec::Hevc, 10, "hevc"},
                         Case{SourceCodec::Av1, 10, "av1"}, Case{SourceCodec::Vp9, 8, "vp9"},
                         Case{SourceCodec::Mpeg2, 8, "mpeg2video"}}) {
        const DecoderChoice d = selectDecoder(stream(c.codec, c.bits), true);
        EXPECT_TRUE(d.try_hardware) << c.name;
        EXPECT_EQ(d.decoder_name, c.name);
        EXPECT_TRUE(d.reason.empty());
    }
}

TEST(DecoderSelection, Av1NatifAvecNvdec_Dav1dSansGpu) {
    EXPECT_EQ(selectDecoder(stream(SourceCodec::Av1, 10), true).decoder_name, "av1");
    const DecoderChoice sw = selectDecoder(stream(SourceCodec::Av1, 10), false);
    EXPECT_FALSE(sw.try_hardware);
    EXPECT_EQ(sw.decoder_name, "libdav1d");
    EXPECT_EQ(sw.reason, "no-nvidia-gpu");
}

TEST(DecoderSelection, ProResEtFfv1EnLogiciel) {
    const DecoderChoice p =
        selectDecoder(stream(SourceCodec::ProRes, 10, ChromaSubsampling::Yuv422), true);
    EXPECT_FALSE(p.try_hardware);
    EXPECT_EQ(p.decoder_name, "prores");
    EXPECT_EQ(p.reason, "nvdec-unsupported-codec");
    EXPECT_EQ(selectDecoder(stream(SourceCodec::Ffv1), true).reason, "nvdec-unsupported-codec");
}

TEST(DecoderSelection, ChromaEtProfondeur) {
    EXPECT_EQ(selectDecoder(stream(SourceCodec::Hevc, 10, ChromaSubsampling::Yuv444), true).reason,
              "nvdec-unsupported-chroma");
    EXPECT_EQ(selectDecoder(stream(SourceCodec::H264, 8, ChromaSubsampling::Yuv422), true).reason,
              "nvdec-unsupported-chroma");
    EXPECT_TRUE(selectDecoder(stream(SourceCodec::Hevc, 12), true).try_hardware);
    EXPECT_EQ(selectDecoder(stream(SourceCodec::Av1, 12), true).reason,
              "nvdec-unsupported-bit-depth");
    EXPECT_EQ(selectDecoder(stream(SourceCodec::Mpeg2, 10), true).reason,
              "nvdec-unsupported-bit-depth");
}

TEST(DecoderSelection, CodecInconnu) {
    const DecoderChoice d = selectDecoder(stream(SourceCodec::Other), true);
    EXPECT_FALSE(d.try_hardware);
    EXPECT_TRUE(d.decoder_name.empty()); // decodeur par defaut de FFmpeg
}

} // namespace
} // namespace enh::pipeline
