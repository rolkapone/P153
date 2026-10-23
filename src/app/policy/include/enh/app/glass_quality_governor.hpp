#pragma once

// Regle de degradation du verre « liquide » (design-system §6.5, ENH-004 specs §5.4).
// C++20 pur, sans Qt : testee en CI sans GPU. RenderPolicy (Qt) lui fournit les mesures.

#include <array>
#include <chrono>
#include <cstddef>
#include <optional>

namespace enh::app {

/// Qualite du verre ; valeurs identiques a Theme.glassFull / glassLite / glassOpaque.
enum class GlassQuality : int {
    Opaque = 0, ///< Rectangle opaque, aucun shader ni capture
    Lite = 1,   ///< flou + teinte + lisere fixe, ni refraction ni reflet mobile
    Full = 2,   ///< refraction, flou, teinte adaptative, lisere, reflet
};

/// Choisit la qualite du verre a partir du temps GPU de rendu de l'interface, image par image.
/// - Hors traitement : toujours Full (et l'historique est oublie).
/// - Pendant un traitement : moyenne glissante sur 30 images ; > 4 ms -> Lite ; > 8 ms -> Opaque ;
///   remontee d'un cran apres 10 s continues sous le budget de 1 ms.
class GlassQualityGovernor {
public:
    using Milliseconds = std::chrono::duration<double, std::milli>;
    using Clock = std::chrono::steady_clock;

    static constexpr std::size_t kWindowFrames = 30;
    static constexpr Milliseconds kLiteAbove{4.0};   ///< design-system §6.5
    static constexpr Milliseconds kOpaqueAbove{8.0}; ///< design-system §6.5
    /// tokens.json glass.budget.uiGpuMsTarget (= Theme.glassGeo.budgetGpuMs).
    static constexpr Milliseconds kRecoverBelow{1.0};
    static constexpr std::chrono::seconds kRecoverAfter{10};
    /// Borne d'un echantillon : une valeur aberrante (pilote, veille) ne fausse pas la moyenne.
    static constexpr Milliseconds kMaxSample{1000.0};

    /// Ajoute une mesure et renvoie la qualite a appliquer. Mesure negative ou non finie : ignoree.
    GlassQuality update(Milliseconds gpuFrameTime, bool processing, Clock::time_point now) noexcept;

    [[nodiscard]] GlassQuality quality() const noexcept { return quality_; }

    /// Moyenne de la fenetre glissante, ou nullopt tant que 30 mesures n'ont pas ete recues.
    [[nodiscard]] std::optional<Milliseconds> average() const noexcept;

private:
    void reset() noexcept;

    std::array<double, kWindowFrames> samples_{};
    std::size_t next_ = 0;
    std::size_t filled_ = 0;
    GlassQuality quality_ = GlassQuality::Full;
    std::optional<Clock::time_point> calmSince_;
};

} // namespace enh::app
