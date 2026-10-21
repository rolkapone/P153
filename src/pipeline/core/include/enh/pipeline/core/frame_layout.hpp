#pragma once

// Disposition des plans NV12 / P010 (SEC-13) : dimensions bornees AVANT tout calcul d'allocation,
// calculs en std::size_t verifies.

#include "enh/pipeline/stage/frame.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace enh::pipeline {

/// Bornes acceptees pour une image source (docs/gpu/ENH-008.md § 1).
inline constexpr std::uint32_t kMinDimensionPx = 1;
inline constexpr std::uint32_t kMaxDimensionPx = 8192;

/// OK si 1 <= largeur, hauteur <= 8192 ; sinon UnsupportedFormat.
[[nodiscard]] Result<void> checkDimensions(Size size_px);

/// Octets par echantillon : 1 (NV12) ou 2 (P010).
[[nodiscard]] constexpr std::size_t bytesPerSample(PixelFormat f) noexcept {
    return f == PixelFormat::P010 ? 2U : 1U;
}

struct FrameLayout {
    /// `address` = decalage du plan depuis le debut du tampon.
    std::array<PlaneView, kMaxPlanes> planes{};
    std::size_t total_bytes = 0;
};

/// Plans Y (largeur x hauteur) puis UV entrelaces (2 x ceil(l/2) echantillons x ceil(h/2) lignes),
/// chaque ligne alignee sur `pitch_alignment` octets (puissance de 2).
[[nodiscard]] Result<FrameLayout> computeFrameLayout(Size size_px, PixelFormat format,
                                                     std::size_t pitch_alignment);

} // namespace enh::pipeline
