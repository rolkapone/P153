#include "enh/platform/appearance.hpp"
#include "enh/platform/window_chrome.hpp"

#include <gtest/gtest.h>

using namespace enh::platform;

TEST(Appearance, ThemeSombreParDefaut) {
    static_assert(darkFromAppsUseLightTheme(std::nullopt));
    static_assert(darkFromAppsUseLightTheme(0U));
    static_assert(!darkFromAppsUseLightTheme(1U));
}

TEST(Appearance, TransparenceActiveParDefaut) {
    static_assert(transparencyFromRegistry(std::nullopt));
    static_assert(transparencyFromRegistry(1U));
    static_assert(!transparencyFromRegistry(0U));
}

TEST(Appearance, ConversionColorref) {
    static_assert(rgbFromColorref(0x00332211U) == 0x112233U);
    static_assert(colorrefFromRgb(0x1B1B1EU) == 0x1E1B1BU);
    static_assert(colorrefFromRgb(rgbFromColorref(0x00ABCDEFU)) == 0x00ABCDEFU);
}

TEST(Appearance, LectureSansPlantage) {
    const Appearance a = readAppearance();
    EXPECT_EQ(a, a);
}

TEST(WindowChrome, MicaDepuisWindows11_22H2) {
    static_assert(!micaSupportedOnBuild(22000));
    static_assert(micaSupportedOnBuild(22621));
    static_assert(micaSupportedOnBuild(26200));
    EXPECT_GT(windowsBuildNumber(), 0U);
}

TEST(WindowChrome, FenetreNulleRefusee) {
    EXPECT_FALSE(applyWindowChrome(nullptr, ChromeState{}));
    const WindowCloak cloak(nullptr);
    EXPECT_FALSE(cloak.cloaked());
}
