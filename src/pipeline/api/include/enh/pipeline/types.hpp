#pragma once

// Types communs du pipeline (docs/gpu/architecture-pipeline.md § 6.1). Aucun type CUDA ni FFmpeg.

#include <cstdint>

namespace enh::pipeline {

/// Taille d'image en pixels.
struct Size {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    friend constexpr bool operator==(const Size&, const Size&) = default;
};

/// Nombre rationnel (cadence, base de temps, rapport d'aspect). Ex. 30000/1001.
struct Rational {
    std::int32_t num = 0;
    std::int32_t den = 1;
    friend constexpr bool operator==(const Rational&, const Rational&) = default;
};

enum class Container : std::uint8_t { Mp4, Mkv };
enum class TrackKind : std::uint8_t { Audio, Subtitle, Attachment, Data };

} // namespace enh::pipeline
