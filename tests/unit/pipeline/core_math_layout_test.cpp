// SEC-13 : calculs de tailles verifies, bornes des dimensions, disposition NV12 / P010.
#include "enh/pipeline/core/checked_math.hpp"
#include "enh/pipeline/core/frame_layout.hpp"

#include <gtest/gtest.h>

#include <limits>

namespace enh::pipeline {
namespace {

constexpr std::size_t kMax = std::numeric_limits<std::size_t>::max();

TEST(CheckedMath, MultiplicationSansDebordement) {
    EXPECT_EQ(*checkedMul(1280, 720), 921600U);
    EXPECT_EQ(*checkedMul(0, kMax), 0U);
    EXPECT_EQ(*checkedMul(kMax, 1), kMax);
}

TEST(CheckedMath, DebordementDetecte) {
    auto r = checkedMul(kMax / 2 + 1, 2);
    ASSERT_FALSE(r);
    EXPECT_EQ(r.error().code, PipelineErrorCode::UnsupportedFormat);
    EXPECT_FALSE(checkedMul(std::size_t{1} << 32, std::size_t{1} << 32));
    EXPECT_FALSE(checkedAdd(kMax, 1));
    EXPECT_EQ(*checkedAdd(kMax - 1, 1), kMax);
}

TEST(CheckedMath, AlignementPuissanceDeDeux) {
    EXPECT_EQ(*alignUp(1, 64), 64U);
    EXPECT_EQ(*alignUp(64, 64), 64U);
    EXPECT_EQ(*alignUp(65, 64), 128U);
    EXPECT_FALSE(alignUp(10, 0));
    EXPECT_FALSE(alignUp(10, 48));
    EXPECT_FALSE(alignUp(kMax, 64));
}

TEST(FrameLayout, BornesDesDimensions) {
    EXPECT_TRUE(checkDimensions(Size{1, 1}));
    EXPECT_TRUE(checkDimensions(Size{8192, 4320}));
    EXPECT_TRUE(checkDimensions(Size{8192, 8192}));
    EXPECT_FALSE(checkDimensions(Size{8193, 4320}));
    EXPECT_FALSE(checkDimensions(Size{1920, 8193}));
    EXPECT_FALSE(checkDimensions(Size{0, 720}));
    EXPECT_FALSE(checkDimensions(Size{0xFFFFFFFFU, 0xFFFFFFFFU}));
    EXPECT_EQ(checkDimensions(Size{8193, 1}).error().code, PipelineErrorCode::UnsupportedFormat);
}

TEST(FrameLayout, Nv12_720p) {
    auto l = computeFrameLayout(Size{1280, 720}, PixelFormat::Nv12, 64);
    ASSERT_TRUE(l);
    EXPECT_EQ(l->planes[0].width_bytes, 1280U);
    EXPECT_EQ(l->planes[0].pitch_bytes, 1280U);
    EXPECT_EQ(l->planes[0].rows, 720U);
    EXPECT_EQ(l->planes[1].address, 1280U * 720U);
    EXPECT_EQ(l->planes[1].width_bytes, 1280U);
    EXPECT_EQ(l->planes[1].rows, 360U);
    EXPECT_EQ(l->total_bytes, 1280U * 720U * 3U / 2U);
}

TEST(FrameLayout, P010_DimensionsImpaires) {
    auto l = computeFrameLayout(Size{1279, 719}, PixelFormat::P010, 64);
    ASSERT_TRUE(l);
    EXPECT_EQ(l->planes[0].width_bytes, 2558U);
    EXPECT_EQ(l->planes[0].pitch_bytes, 2560U);
    EXPECT_EQ(l->planes[1].width_bytes, 2560U); // ceil(1279/2) * 2 echantillons * 2 octets
    EXPECT_EQ(l->planes[1].rows, 360U);         // ceil(719/2)
    EXPECT_EQ(l->total_bytes, 2560U * 719U + 2560U * 360U);
}

TEST(FrameLayout, UnSurUn_EtMaximum) {
    auto one = computeFrameLayout(Size{1, 1}, PixelFormat::Nv12, 1);
    ASSERT_TRUE(one);
    EXPECT_EQ(one->planes[0].width_bytes, 1U);
    EXPECT_EQ(one->planes[1].width_bytes, 2U);
    EXPECT_EQ(one->total_bytes, 3U);
    auto big = computeFrameLayout(Size{8192, 4320}, PixelFormat::P010, 256);
    ASSERT_TRUE(big);
    EXPECT_EQ(big->total_bytes, 16384U * 4320U + 16384U * 2160U);
    EXPECT_FALSE(computeFrameLayout(Size{8193, 4320}, PixelFormat::P010, 256));
}

TEST(PipelineError, EmplacementSansCheminDeBuild) {
    const PipelineError e = makeError(PipelineErrorCode::Internal, "boom");
    EXPECT_NE(e.detail.find("boom ["), std::string::npos);
    EXPECT_NE(e.detail.find("tests"), std::string::npos);
    EXPECT_EQ(e.detail.find(":\\"), std::string::npos); // pas de lettre de lecteur
    EXPECT_EQ(toString(PipelineErrorCode::SourceMissing), "SourceMissing");
}

} // namespace
} // namespace enh::pipeline
