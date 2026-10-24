// ENH-047 : lecteur BMP durci (SEC-13) et conversions vers les formats du RTX Video SDK.
#include "enh/bench/bmp.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <vector>

namespace enh::bench {
namespace {

using pipeline::PipelineErrorCode;
using pipeline::Size;

RgbImage pattern(std::uint32_t w, std::uint32_t h) {
    RgbImage img;
    img.size_px = {w, h};
    img.rgb.resize(std::size_t{w} * h * 3);
    for (std::size_t i = 0; i < img.rgb.size(); ++i) {
        img.rgb[i] = static_cast<std::uint8_t>((i * 37U + 11U) & 0xFFU);
    }
    return img;
}

void put32(std::vector<std::byte>& b, std::size_t at, std::uint32_t v) {
    for (unsigned k = 0; k < 4; ++k) {
        b[at + k] = static_cast<std::byte>((v >> (8U * k)) & 0xFFU);
    }
}

void put16(std::vector<std::byte>& b, std::size_t at, std::uint16_t v) {
    b[at] = static_cast<std::byte>(v & 0xFFU);
    b[at + 1] = static_cast<std::byte>(v >> 8U);
}

/// BMP 32 bits construit a la main (BI_RGB ou BI_BITFIELDS, haut-bas si top_down).
std::vector<std::byte> make32(const RgbImage& img, bool top_down, bool bitfields,
                              std::uint32_t dib = 40) {
    const std::uint32_t w = img.size_px.width;
    const std::uint32_t h = img.size_px.height;
    const std::size_t masks = (bitfields && dib == 40) ? 12 : 0;
    const std::size_t off = 14 + dib + masks;
    std::vector<std::byte> b(off + std::size_t{w} * h * 4, std::byte{0});
    b[0] = std::byte{'B'};
    b[1] = std::byte{'M'};
    put32(b, 2, static_cast<std::uint32_t>(b.size()));
    put32(b, 10, static_cast<std::uint32_t>(off));
    put32(b, 14, dib);
    put32(b, 18, w);
    put32(b, 22, top_down ? static_cast<std::uint32_t>(-static_cast<std::int32_t>(h)) : h);
    put16(b, 26, 1);
    put16(b, 28, 32);
    put32(b, 30, bitfields ? 3U : 0U);
    if (bitfields) {
        put32(b, 54, 0x00FF0000U);
        put32(b, 58, 0x0000FF00U);
        put32(b, 62, 0x000000FFU);
    }
    for (std::size_t y = 0; y < h; ++y) {
        const std::size_t row = top_down ? y : h - 1 - y;
        for (std::size_t x = 0; x < w; ++x) {
            const std::uint8_t* px = img.rgb.data() + (y * w + x) * 3;
            std::byte* d = b.data() + off + (row * w + x) * 4;
            d[0] = static_cast<std::byte>(px[2]);
            d[1] = static_cast<std::byte>(px[1]);
            d[2] = static_cast<std::byte>(px[0]);
            d[3] = std::byte{0xFF};
        }
    }
    return b;
}

TEST(BenchBmp, AllerRetour24Bits_TaillesImpaires) {
    for (const Size s : {Size{1, 1}, Size{3, 5}, Size{7, 2}, Size{1281, 721}}) {
        const RgbImage img = pattern(s.width, s.height);
        auto enc = encodeBmp24(img);
        ASSERT_TRUE(enc) << enc.error().detail;
        // Lignes alignees sur 4 octets.
        const std::size_t stride = (std::size_t{s.width} * 3 + 3) / 4 * 4;
        EXPECT_EQ(enc->size(), 54 + stride * s.height);
        auto dec = decodeBmp(*enc);
        ASSERT_TRUE(dec) << dec.error().detail;
        EXPECT_EQ(dec->size_px, s);
        EXPECT_EQ(dec->rgb, img.rgb) << s.width << "x" << s.height;
    }
}

TEST(BenchBmp, Lecture32Bits_BasHaut_HautBas_Bitfields_V5) {
    const RgbImage img = pattern(5, 3);
    for (const bool top_down : {false, true}) {
        for (const bool bitfields : {false, true}) {
            for (const std::uint32_t dib : {40U, 124U}) {
                auto dec = decodeBmp(make32(img, top_down, bitfields, dib));
                ASSERT_TRUE(dec) << dec.error().detail << " top_down=" << top_down
                                 << " bitfields=" << bitfields << " dib=" << dib;
                EXPECT_EQ(dec->rgb, img.rgb);
            }
        }
    }
}

struct Corruption {
    const char* name;
    void (*mutate)(std::vector<std::byte>&);
    PipelineErrorCode expected;
};

class BenchBmpMalforme : public ::testing::TestWithParam<Corruption> {};

TEST_P(BenchBmpMalforme, RefuseSansPlantage) {
    auto enc = encodeBmp24(pattern(9, 7));
    ASSERT_TRUE(enc);
    std::vector<std::byte> b = *enc;
    GetParam().mutate(b);
    auto dec = decodeBmp(b);
    ASSERT_FALSE(dec) << GetParam().name;
    EXPECT_EQ(dec.error().code, GetParam().expected)
        << GetParam().name << " : " << dec.error().detail;
}

INSTANTIATE_TEST_SUITE_P(
    Cas, BenchBmpMalforme,
    ::testing::Values(
        Corruption{"vide", [](std::vector<std::byte>& b) { b.clear(); },
                   PipelineErrorCode::SourceCorrupt},
        Corruption{"entete_tronque", [](std::vector<std::byte>& b) { b.resize(30); },
                   PipelineErrorCode::SourceCorrupt},
        Corruption{"donnees_tronquees", [](std::vector<std::byte>& b) { b.resize(b.size() - 1); },
                   PipelineErrorCode::SourceCorrupt},
        Corruption{"signature", [](std::vector<std::byte>& b) { b[0] = std::byte{'P'}; },
                   PipelineErrorCode::UnsupportedFormat},
        Corruption{"largeur_nulle", [](std::vector<std::byte>& b) { put32(b, 18, 0); },
                   PipelineErrorCode::SourceCorrupt},
        Corruption{"largeur_negative", [](std::vector<std::byte>& b) { put32(b, 18, 0xFFFFFFF0U); },
                   PipelineErrorCode::SourceCorrupt},
        Corruption{"largeur_8193", [](std::vector<std::byte>& b) { put32(b, 18, 8193); },
                   PipelineErrorCode::SourceCorrupt},
        Corruption{"largeur_enorme", [](std::vector<std::byte>& b) { put32(b, 18, 0x7FFFFFFFU); },
                   PipelineErrorCode::SourceCorrupt},
        Corruption{"hauteur_nulle", [](std::vector<std::byte>& b) { put32(b, 22, 0); },
                   PipelineErrorCode::SourceCorrupt},
        Corruption{"hauteur_int_min", [](std::vector<std::byte>& b) { put32(b, 22, 0x80000000U); },
                   PipelineErrorCode::SourceCorrupt},
        Corruption{"hauteur_8192_donnees_absentes",
                   [](std::vector<std::byte>& b) { put32(b, 22, 8192); },
                   PipelineErrorCode::SourceCorrupt},
        Corruption{"plans", [](std::vector<std::byte>& b) { put16(b, 26, 2); },
                   PipelineErrorCode::SourceCorrupt},
        Corruption{"8_bits_palette", [](std::vector<std::byte>& b) { put16(b, 28, 8); },
                   PipelineErrorCode::UnsupportedFormat},
        Corruption{"16_bits", [](std::vector<std::byte>& b) { put16(b, 28, 16); },
                   PipelineErrorCode::UnsupportedFormat},
        Corruption{"rle8", [](std::vector<std::byte>& b) { put32(b, 30, 1); },
                   PipelineErrorCode::UnsupportedFormat},
        Corruption{"bitfields_24_bits", [](std::vector<std::byte>& b) { put32(b, 30, 3); },
                   PipelineErrorCode::UnsupportedFormat},
        Corruption{"core_header_12", [](std::vector<std::byte>& b) { put32(b, 14, 12); },
                   PipelineErrorCode::UnsupportedFormat},
        Corruption{"dib_enorme", [](std::vector<std::byte>& b) { put32(b, 14, 0xFFFFFFFFU); },
                   PipelineErrorCode::UnsupportedFormat},
        Corruption{"offset_dans_entete", [](std::vector<std::byte>& b) { put32(b, 10, 20); },
                   PipelineErrorCode::SourceCorrupt},
        Corruption{"offset_hors_fichier",
                   [](std::vector<std::byte>& b) { put32(b, 10, 0xFFFFFFF0U); },
                   PipelineErrorCode::SourceCorrupt}),
    [](const ::testing::TestParamInfo<Corruption>& info) { return std::string(info.param.name); });

TEST(BenchBmp, Bitfields_MasquesExotiques_Refuses) {
    auto b = make32(pattern(2, 2), false, true);
    put32(b, 54, 0x000000FFU); // R et B inverses
    put32(b, 62, 0x00FF0000U);
    auto dec = decodeBmp(b);
    ASSERT_FALSE(dec);
    EXPECT_EQ(dec.error().code, PipelineErrorCode::UnsupportedFormat);
}

TEST(BenchBmp, Bitfields_MasquesTronques_Refuses) {
    auto b = make32(pattern(1, 1), false, true);
    b.resize(60); // en-tete de 40 + 6 octets de masques
    auto dec = decodeBmp(b);
    ASSERT_FALSE(dec);
    EXPECT_EQ(dec.error().code, PipelineErrorCode::SourceCorrupt);
}

TEST(BenchBmp, EncodageRefuseUneImageIncoherente) {
    RgbImage img = pattern(4, 4);
    img.rgb.pop_back();
    EXPECT_FALSE(encodeBmp24(img));
    EXPECT_FALSE(encodeBmp24(RgbImage{}));
}

TEST(BenchBmp, ConversionsArgb8_EtAbgr10) {
    RgbImage img;
    img.size_px = {3, 1};
    img.rgb = {0, 128, 255, 255, 0, 1, 10, 20, 30};
    const auto argb = toArgb8(img);
    ASSERT_EQ(argb.size(), 3U);
    EXPECT_EQ(argb[0], 0xFFFF8000U); // A=FF, B=FF, G=80, R=00
    EXPECT_EQ(fromArgb8(argb, img.size_px).rgb, img.rgb);

    const auto abgr = toAbgr10(img);
    EXPECT_EQ(abgr[0] & 0x3FFU, 0U);
    EXPECT_EQ((abgr[0] >> 10U) & 0x3FFU, 514U); // round(128 * 1023 / 255)
    EXPECT_EQ((abgr[0] >> 20U) & 0x3FFU, 1023U);
    EXPECT_EQ(abgr[0] >> 30U, 3U);
    // Aller-retour 8 -> 10 -> 8 bits sans perte.
    EXPECT_EQ(fromAbgr10(abgr, img.size_px).rgb, img.rgb);
    RgbImage all;
    all.size_px = {256, 1};
    for (int v = 0; v < 256; ++v) {
        all.rgb.insert(all.rgb.end(), 3, static_cast<std::uint8_t>(v));
    }
    EXPECT_EQ(fromAbgr10(toAbgr10(all), all.size_px).rgb, all.rgb);
}

} // namespace
} // namespace enh::bench
