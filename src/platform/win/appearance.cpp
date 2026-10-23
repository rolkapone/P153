#include "enh/platform/appearance.hpp"

#include "win_include.hpp"

namespace enh::platform {
namespace {

constexpr const wchar_t* kPersonalizeKey =
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";

std::optional<std::uint32_t> readUserDword(const wchar_t* valueName) noexcept {
    DWORD value = 0;
    DWORD size = sizeof(value);
    const LSTATUS status = ::RegGetValueW(HKEY_CURRENT_USER, kPersonalizeKey, valueName,
                                          RRF_RT_REG_DWORD, nullptr, &value, &size);
    if (status != ERROR_SUCCESS) {
        return std::nullopt;
    }
    return static_cast<std::uint32_t>(value);
}

std::uint32_t sysColor(int index) noexcept {
    return rgbFromColorref(static_cast<std::uint32_t>(::GetSysColor(index)));
}

} // namespace

Appearance readAppearance() noexcept {
    Appearance a;
    a.dark = darkFromAppsUseLightTheme(readUserDword(L"AppsUseLightTheme"));
    a.transparency = transparencyFromRegistry(readUserDword(L"EnableTransparency"));

    HIGHCONTRASTW hc{};
    hc.cbSize = sizeof(hc);
    if (::SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(hc), &hc, 0) != FALSE) {
        a.highContrast = (hc.dwFlags & HCF_HIGHCONTRASTON) != 0;
    }

    BOOL animations = TRUE;
    if (::SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION, 0, &animations, 0) != FALSE) {
        a.reduceMotion = animations == FALSE;
    }

    a.colors.window = sysColor(COLOR_WINDOW);
    a.colors.windowText = sysColor(COLOR_WINDOWTEXT);
    a.colors.buttonFace = sysColor(COLOR_BTNFACE);
    a.colors.buttonText = sysColor(COLOR_BTNTEXT);
    a.colors.highlight = sysColor(COLOR_HIGHLIGHT);
    a.colors.highlightText = sysColor(COLOR_HIGHLIGHTTEXT);
    a.colors.grayText = sysColor(COLOR_GRAYTEXT);
    return a;
}

} // namespace enh::platform
