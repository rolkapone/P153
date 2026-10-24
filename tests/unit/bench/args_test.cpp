// ENH-047 : analyse des arguments de enhance-bench (valeurs par defaut, erreurs, combinaisons).
#include "enh/bench/args.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace enh::bench {
namespace {

enh::Expected<BenchArgs, std::string> parse(std::vector<std::wstring> a) {
    return parseArgs(a);
}

TEST(BenchArgs, Decode_ValeursParDefaut) {
    auto r = parse({L"--stage", L"decode", L"--in", L"clip.mp4"});
    ASSERT_TRUE(r) << r.error();
    EXPECT_EQ(r->stage, Stage::Decode);
    EXPECT_EQ(r->input, std::filesystem::path(L"clip.mp4"));
    EXPECT_FALSE(r->report.has_value());
    EXPECT_FALSE(r->output.has_value());
    EXPECT_EQ(r->repeat, 1U);
    EXPECT_EQ(r->warmup, 30U);
    EXPECT_EQ(r->max_frames, 0U);
    EXPECT_EQ(r->decoder, DecoderMode::Auto);
    EXPECT_EQ(r->sample_ms, 10U);
    EXPECT_FALSE(r->force);
    EXPECT_FALSE(r->help);
}

TEST(BenchArgs, VsrImage_ValeursParDefaut) {
    auto r = parse({L"--stage", L"vsr-image", L"--in", L"a.bmp", L"--out", L"b.bmp"});
    ASSERT_TRUE(r) << r.error();
    EXPECT_EQ(r->stage, Stage::VsrImage);
    EXPECT_EQ(r->quality, 4);
    EXPECT_EQ(r->bits, 8);
    EXPECT_EQ(r->size, (pipeline::Size{.width = 3840, .height = 2160}));
    EXPECT_EQ(r->iterations, 300U);
    EXPECT_EQ(r->warmup, 30U);
    ASSERT_TRUE(r->output.has_value());
    EXPECT_EQ(*r->output, std::filesystem::path(L"b.bmp"));
}

TEST(BenchArgs, Decode_TousLesArguments_EtFormeEgal) {
    auto r = parse({L"--stage=decode", L"--in", L"vidéo 日本.mkv", L"--report", L"r.json",
                    L"--repeat", L"10", L"--warmup=0", L"--max-frames", L"500", L"--decoder",
                    L"software", L"--sample-ms", L"5", L"--force"});
    ASSERT_TRUE(r) << r.error();
    EXPECT_EQ(r->input, std::filesystem::path(L"vidéo 日本.mkv"));
    ASSERT_TRUE(r->report.has_value());
    EXPECT_EQ(*r->report, std::filesystem::path(L"r.json"));
    EXPECT_EQ(r->repeat, 10U);
    EXPECT_EQ(r->warmup, 0U);
    EXPECT_EQ(r->max_frames, 500U);
    EXPECT_EQ(r->decoder, DecoderMode::Software);
    EXPECT_EQ(r->sample_ms, 5U);
    EXPECT_TRUE(r->force);
}

TEST(BenchArgs, VsrImage_TousLesArguments) {
    auto r = parse({L"--stage", L"vsr-image", L"--in", L"a.bmp", L"--out", L"b.bmp", L"--q", L"0",
                    L"--bits", L"10", L"--size", L"1920X1080", L"--iterations", L"5"});
    ASSERT_TRUE(r) << r.error();
    EXPECT_EQ(r->quality, 0);
    EXPECT_EQ(r->bits, 10);
    EXPECT_EQ(r->size, (pipeline::Size{.width = 1920, .height = 1080}));
    EXPECT_EQ(r->iterations, 5U);
}

TEST(BenchArgs, Aide) {
    for (const wchar_t* h : {L"--help", L"-h", L"/?"}) {
        auto r = parse({h});
        ASSERT_TRUE(r) << r.error();
        EXPECT_TRUE(r->help);
    }
    EXPECT_NE(usageText().find("--stage decode"), std::string_view::npos);
    EXPECT_NE(usageText().find("Codes de sortie"), std::string_view::npos);
}

struct BadCase {
    std::vector<std::wstring> args;
    const char* expected_fragment;
};

class BenchArgsErreurs : public ::testing::TestWithParam<BadCase> {};

TEST_P(BenchArgsErreurs, Refuse_AvecMessage) {
    const BadCase& c = GetParam();
    auto r = parseArgs(c.args);
    ASSERT_FALSE(r);
    EXPECT_NE(r.error().find(c.expected_fragment), std::string::npos) << r.error();
}

INSTANTIATE_TEST_SUITE_P(
    Cas, BenchArgsErreurs,
    ::testing::Values(
        BadCase{{}, "--stage est obligatoire"},
        BadCase{{L"--in", L"a.mp4"}, "--stage est obligatoire"},
        BadCase{{L"--stage", L"decode"}, "--in est obligatoire"},
        BadCase{{L"--stage", L"encode", L"--in", L"a"}, "decode ou vsr-image"},
        BadCase{{L"--stage", L"decode", L"--in"}, "valeur manquante"},
        BadCase{{L"--stage", L"decode", L"--in", L"a", L"--bogus"}, "option inconnue"},
        BadCase{{L"--stage", L"decode", L"--in", L"a", L"--in", L"b"}, "option repetee"},
        BadCase{{L"--stage", L"decode", L"--in", L"a", L"--repeat", L"0"}, "--repeat"},
        BadCase{{L"--stage", L"decode", L"--in", L"a", L"--repeat", L"1001"}, "--repeat"},
        BadCase{{L"--stage", L"decode", L"--in", L"a", L"--repeat", L"-3"}, "--repeat"},
        BadCase{{L"--stage", L"decode", L"--in", L"a", L"--repeat", L"3x"}, "--repeat"},
        BadCase{{L"--stage", L"decode", L"--in", L"a", L"--warmup", L"99999999999999999999"},
                "--warmup"},
        BadCase{{L"--stage", L"decode", L"--in", L"a", L"--sample-ms", L"0"}, "--sample-ms"},
        BadCase{{L"--stage", L"decode", L"--in", L"a", L"--decoder", L"cuda"}, "--decoder"},
        BadCase{{L"--stage", L"decode", L"--in", L"a", L"--force=1"}, "ne prend pas de valeur"},
        BadCase{{L"--stage", L"decode", L"--in", L"a", L"--out", L"b"}, "--out ne s'applique"},
        BadCase{{L"--stage", L"decode", L"--in", L"a", L"--q", L"4"}, "--q ne s'applique"},
        BadCase{{L"--stage", L"vsr-image", L"--in", L"a.bmp"}, "--out est obligatoire"},
        BadCase{{L"--stage", L"vsr-image", L"--in", L"a", L"--out", L"b", L"--decoder", L"auto"},
                "--decoder ne s'applique"},
        BadCase{{L"--stage", L"vsr-image", L"--in", L"a", L"--out", L"b", L"--q", L"5"}, "--q"},
        BadCase{{L"--stage", L"vsr-image", L"--in", L"a", L"--out", L"b", L"--bits", L"12"},
                "--bits"},
        BadCase{{L"--stage", L"vsr-image", L"--in", L"a", L"--out", L"b", L"--size", L"3840"},
                "--size"},
        BadCase{{L"--stage", L"vsr-image", L"--in", L"a", L"--out", L"b", L"--size", L"0x10"},
                "--size"},
        BadCase{{L"--stage", L"vsr-image", L"--in", L"a", L"--out", L"b", L"--size", L"9000x10"},
                "--size"},
        BadCase{{L"--stage", L"vsr-image", L"--in", L"a", L"--out", L"x", L"--report", L"x"},
                "meme fichier"},
        BadCase{{L"--stage", L"decode", L"--in", L""}, "--in"}));

TEST(BenchFitInside, ConserveLeRapportDAspect) {
    using pipeline::Size;
    EXPECT_EQ(fitInside({1280, 720}, {3840, 2160}), (Size{3840, 2160}));
    EXPECT_EQ(fitInside({1280, 570}, {3840, 2160}), (Size{3840, 1710}));
    EXPECT_EQ(fitInside({720, 1280}, {3840, 2160}), (Size{1215, 2160}));
    EXPECT_EQ(fitInside({1, 1}, {7, 3}), (Size{3, 3}));
    EXPECT_EQ(fitInside({0, 10}, {7, 3}), (Size{7, 3}));
}

} // namespace
} // namespace enh::bench
