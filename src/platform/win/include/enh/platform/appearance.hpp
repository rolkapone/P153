#pragma once

// Reglages d'apparence de Windows (theme, transparence, contraste eleve, animations).
// Couche Win32 sans Qt (ADR-036) : la presentation (SystemAppearance) les relaie au QML.

#include <cstdint>
#include <optional>

namespace enh::platform {

/// Couleurs systeme (GetSysColor), en 0xRRGGBB. Utilisees en contraste eleve.
struct SystemColors {
    std::uint32_t window = 0;
    std::uint32_t windowText = 0;
    std::uint32_t buttonFace = 0;
    std::uint32_t buttonText = 0;
    std::uint32_t highlight = 0;
    std::uint32_t highlightText = 0;
    std::uint32_t grayText = 0;

    friend bool operator==(const SystemColors&, const SystemColors&) = default;
};

/// Instantane des reglages Windows qui changent le rendu de l'interface.
struct Appearance {
    bool dark = true;          ///< AppsUseLightTheme == 0
    bool transparency = true;  ///< EnableTransparency != 0 (« Effets de transparence »)
    bool highContrast = false; ///< SPI_GETHIGHCONTRAST, HCF_HIGHCONTRASTON
    bool reduceMotion = false; ///< SPI_GETCLIENTAREAANIMATION == FALSE (« Effets d'animation »)
    /// DWM\ColorPrevalence != 0 : « Afficher la couleur d'accentuation sur les barres de titre ».
    /// Choix de l'utilisateur, respecte : l'app n'impose alors pas sa couleur de barre de titre.
    bool accentOnTitleBars = false;
    SystemColors colors{};

    friend bool operator==(const Appearance&, const Appearance&) = default;
};

/// Lit les reglages courants (registre HKCU + SystemParametersInfo + GetSysColor).
/// Ne bloque pas (lectures locales) ; toute valeur illisible prend sa valeur par defaut.
[[nodiscard]] Appearance readAppearance() noexcept;

/// AppsUseLightTheme : 0 = sombre ; valeur absente = sombre (defaut du design system).
[[nodiscard]] constexpr bool
darkFromAppsUseLightTheme(std::optional<std::uint32_t> value) noexcept {
    return !value.has_value() || *value == 0;
}

/// EnableTransparency : 0 = desactivee ; valeur absente = activee (defaut de Windows 11).
[[nodiscard]] constexpr bool transparencyFromRegistry(std::optional<std::uint32_t> value) noexcept {
    return !value.has_value() || *value != 0;
}

/// DWM\ColorPrevalence : 1 = couleur d'accentuation sur les barres de titre ; absente = non.
[[nodiscard]] constexpr bool
accentOnTitleBarsFromRegistry(std::optional<std::uint32_t> value) noexcept {
    return value.has_value() && *value != 0;
}

/// COLORREF (0x00BBGGRR) vers 0xRRGGBB.
[[nodiscard]] constexpr std::uint32_t rgbFromColorref(std::uint32_t colorref) noexcept {
    const std::uint32_t r = colorref & 0xFFU;
    const std::uint32_t g = (colorref >> 8U) & 0xFFU;
    const std::uint32_t b = (colorref >> 16U) & 0xFFU;
    return (r << 16U) | (g << 8U) | b;
}

/// 0xRRGGBB vers COLORREF (0x00BBGGRR).
[[nodiscard]] constexpr std::uint32_t colorrefFromRgb(std::uint32_t rgb) noexcept {
    return rgbFromColorref(rgb); // l'echange R <-> B est sa propre reciproque
}

} // namespace enh::platform
