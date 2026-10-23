#include "enh/app/app_options.hpp"

#include <gtest/gtest.h>

#include <array>
#include <string_view>

using enh::app::AppOptions;
using enh::app::GlassQuality;
using enh::app::parseAppOptions;
using enh::app::parseGlassQuality;

TEST(AppOptions, SansArgumentValeursParDefaut) {
    EXPECT_EQ(parseAppOptions({}), AppOptions{});
}

TEST(AppOptions, ReconnaitToutesLesOptions) {
    constexpr std::array<std::string_view, 4> args{"--glass-testbed", "--glass-quality=1",
                                                   "--no-mica", "--check-modules"};
    const AppOptions o = parseAppOptions(args);
    EXPECT_TRUE(o.glassTestbed);
    EXPECT_EQ(o.forcedGlassQuality, GlassQuality::Lite);
    EXPECT_TRUE(o.noMica);
    EXPECT_TRUE(o.checkModules);
}

TEST(AppOptions, QualiteHorsBornesIgnoree) {
    for (const std::string_view bad :
         {"--glass-quality=3", "--glass-quality=-1", "--glass-quality=", "--glass-quality=2x",
          "--glass-quality=02"}) {
        const std::array<std::string_view, 1> args{bad};
        EXPECT_FALSE(parseAppOptions(args).forcedGlassQuality.has_value()) << bad;
    }
}

TEST(AppOptions, DerniereQualiteValideGagne) {
    constexpr std::array<std::string_view, 3> args{"--glass-quality=0", "--glass-quality=9",
                                                   "--glass-quality=2"};
    EXPECT_EQ(parseAppOptions(args).forcedGlassQuality, GlassQuality::Full);
}

TEST(AppOptions, ArgumentsInconnusIgnores) {
    constexpr std::array<std::string_view, 3> args{"--inconnu", "fichier.mp4", "-glass-testbed"};
    EXPECT_EQ(parseAppOptions(args), AppOptions{});
}

TEST(AppOptions, ParseQualite) {
    EXPECT_EQ(parseGlassQuality("0"), GlassQuality::Opaque);
    EXPECT_EQ(parseGlassQuality("1"), GlassQuality::Lite);
    EXPECT_EQ(parseGlassQuality("2"), GlassQuality::Full);
    EXPECT_FALSE(parseGlassQuality("deux").has_value());
}
