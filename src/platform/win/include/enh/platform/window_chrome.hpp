#pragma once

// Habillage DWM de la fenetre principale : Mica, barre de titre sombre/claire, masquage
// jusqu'a la premiere image (ADR-001, ENH-004 specs §3.2). Couche Win32 sans Qt.

#include <cstdint>
#include <optional>

struct HWND__; // HWND sans inclure <windows.h> dans un en-tete public

namespace enh::platform {

using NativeWindow = HWND__*;

/// Etat d'habillage voulu pour une fenetre.
struct ChromeState {
    bool dark = true;          ///< barre de titre sombre (DWMWA_USE_IMMERSIVE_DARK_MODE)
    bool mica = true;          ///< fond Mica (DWMSBT_MAINWINDOW) et cadre etendu a la zone client
    bool highContrast = false; ///< contraste eleve : couleurs de legende laissees au systeme
    /// Couleur de la barre de titre en mode opaque (0xRRGGBB), pour qu'il n'y ait aucune couture
    /// avec le contenu ; nullopt = couleur par defaut de Windows.
    std::optional<std::uint32_t> captionRgb;

    friend bool operator==(const ChromeState&, const ChromeState&) = default;
};

/// Numero de build de Windows (RtlGetVersion : non affecte par le manifeste de compatibilite).
[[nodiscard]] std::uint32_t windowsBuildNumber() noexcept;

/// Mica par DWMWA_SYSTEMBACKDROP_TYPE : Windows 11 22H2 (build 22621) ou plus.
[[nodiscard]] constexpr bool micaSupportedOnBuild(std::uint32_t build) noexcept {
    return build >= 22621U;
}

/// Applique les attributs DWM. Renvoie false si un Mica demande a ete refuse par DWM
/// (l'appelant passe alors en repli opaque) ; true sinon.
bool applyWindowChrome(NativeWindow window, const ChromeState& state) noexcept;

/// Filtre de messages de la fenetre `window` (MSG Win32 passe en `const void*`) : sur
/// WM_ERASEBKGND, peint le fond en noir GDI, rendu transparent par DWM dans un cadre etendu
/// (sinon un fond blanc masque le Mica derriere la surface DirectComposition de Qt).
/// Renvoie true (et *result = 1) si le message est traite.
bool handleBackdropMessage(NativeWindow window, const void* message, long long* result) noexcept;

/// RAII : masque la fenetre (DWMWA_CLOAK) a la construction, la devoile au plus tard a la
/// destruction. Ainsi la premiere image visible est complete (aucun flash blanc), et une
/// fenetre ne peut jamais rester invisible si le code qui devait la devoiler echoue.
class WindowCloak {
public:
    explicit WindowCloak(NativeWindow window) noexcept;
    ~WindowCloak();
    WindowCloak(const WindowCloak&) = delete;
    WindowCloak& operator=(const WindowCloak&) = delete;
    WindowCloak(WindowCloak&&) = delete;
    WindowCloak& operator=(WindowCloak&&) = delete;

    /// Devoile la fenetre (sans effet si c'est deja fait).
    void reveal() noexcept;
    [[nodiscard]] bool cloaked() const noexcept { return cloaked_; }

private:
    NativeWindow window_;
    bool cloaked_ = false;
};

} // namespace enh::platform
