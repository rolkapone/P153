#pragma once

// BMP non compresse (docs/gpu/ENH-047.md § 5) et conversions vers les formats du RTX Video SDK.
// Lecture durcie (SEC-13) : 24 bits BI_RGB, 32 bits BI_RGB ou BI_BITFIELDS (masques standard),
// de bas en haut ou de haut en bas ; 1 <= largeur, hauteur <= 8192 ; tailles calculees avec
// checkedMul / checkedAdd ; donnees dans les bornes du fichier. Ecriture : 24 bits, de bas en haut.

#include "enh/pipeline/error.hpp"
#include "enh/pipeline/types.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace enh::bench {

inline constexpr std::uint32_t kMaxBmpSidePx = 8192;
/// Plus gros fichier BMP accepte en lecture : 8192 x 8192 x 4 + en-tetes (256 Mio + marge).
inline constexpr std::size_t kMaxBmpFileBytes = (std::size_t{8192} * 8192 * 4) + 4096;

/// Image 8 bits par composante en memoire CPU : 3 octets par pixel, ordre R, G, B, lignes
/// contigues de haut en bas.
struct RgbImage {
    pipeline::Size size_px;
    std::vector<std::uint8_t> rgb;
};

/// Decode un fichier BMP deja lu en memoire. UnsupportedFormat : variante non prise en charge
/// (compression, palette, 16 bits, masques exotiques) ; SourceCorrupt : en-tete incoherent,
/// tronque, dimensions hors bornes.
[[nodiscard]] pipeline::Result<RgbImage> decodeBmp(std::span<const std::byte> file);

/// Encode en BMP 24 bits BI_RGB de bas en haut (lisible par Windows Photos, Paint, FFmpeg).
[[nodiscard]] pipeline::Result<std::vector<std::byte>> encodeBmp24(const RgbImage& image);

/// RgbImage -> pixels Argb8 du SDK (octets R, G, B, A=255 ; R dans l'octet de poids faible).
[[nodiscard]] std::vector<std::uint32_t> toArgb8(const RgbImage& image);
/// RgbImage -> pixels Abgr10 (R10G10B10A2, R dans les bits 0-9 ; v10 = round(v8 * 1023 / 255)).
[[nodiscard]] std::vector<std::uint32_t> toAbgr10(const RgbImage& image);
/// Pixels Argb8 -> RgbImage.
[[nodiscard]] RgbImage fromArgb8(std::span<const std::uint32_t> pixels, pipeline::Size size_px);
/// Pixels Abgr10 -> RgbImage (v8 = round(v10 * 255 / 1023)).
[[nodiscard]] RgbImage fromAbgr10(std::span<const std::uint32_t> pixels, pipeline::Size size_px);

} // namespace enh::bench
