#pragma once

// Super-resolution (interface INTERNE au pipeline, docs/gpu/ENH-040.md § 2 et § 7).
// Portee d'ENH-040 : images RGB deja sur le GPU, lues par une texture CUDA et ecrites par une
// surface CUDA. Le branchement sur VideoFrame (NV12 / P010) et les conversions couleur viennent
// avec ENH-042 / ENH-009. Implementations : RtxVideoUpscaler (RTX Video SDK, GPU) et un faux pour
// la CI.

#include "enh/pipeline/error.hpp"
#include "enh/pipeline/types.hpp"

#include <cstdint>

namespace enh::pipeline {

/// Niveaux du RTX Video SDK (NVSDK_NGX_VSR_QualityLevel) : 0 = bicubique ... 4 = Ultra.
enum class UpscaleQuality : std::uint8_t { Bicubic = 0, Low = 1, Medium = 2, High = 3, Ultra = 4 };

/// Format des images d'entree et de sortie (le meme des deux cotes).
/// Argb8 : 4 x 8 bits par pixel, octets R, G, B, A (x = R) ; Abgr10 : R10G10B10A2 non signe
/// normalise (x2bgr10le de FFmpeg). Convention du RTX Video SDK (samples/ReadMe.md).
enum class RgbFormat : std::uint8_t { Argb8, Abgr10 };

struct UpscaleRequest {
    std::uint64_t input_texture = 0;  // CUtexObject : coordonnees normalisees, filtrage lineaire
    Size input_size_px;               // zone lue, depuis (0, 0)
    std::uint64_t output_surface = 0; // CUsurfObject (tableau CUDA en CUDA_ARRAY3D_SURFACE_LDST)
    Size output_size_px;              // zone ecrite, depuis (0, 0)
    UpscaleQuality quality = UpscaleQuality::Ultra;
};

class IUpscaler {
public:
    IUpscaler() = default;
    IUpscaler(const IUpscaler&) = delete;
    IUpscaler& operator=(const IUpscaler&) = delete;
    IUpscaler(IUpscaler&&) = delete;
    IUpscaler& operator=(IUpscaler&&) = delete;
    virtual ~IUpscaler() = default;

    /// Met en file la super-resolution sur le flux CUDA donne a la creation de l'upscaler. Ne
    /// synchronise pas : l'appelant enregistre un evenement CUDA apres l'appel (ADR-045). Pas
    /// d'allocation en cas de succes (boucle chaude). Un seul thread a la fois par upscaler.
    [[nodiscard]] virtual Result<void> upscale(const UpscaleRequest& request) noexcept = 0;
};

} // namespace enh::pipeline
