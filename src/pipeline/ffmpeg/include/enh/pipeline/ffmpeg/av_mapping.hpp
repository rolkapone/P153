#pragma once

// Correspondances pures FFmpeg -> types du pipeline (couleur, sous-echantillonnage).

#include "enh/pipeline/stage/demuxer.hpp"
#include "enh/pipeline/stage/frame.hpp"

extern "C" {
#include <libavutil/pixfmt.h>
#include <libavutil/rational.h>
}

namespace enh::pipeline::ffmpeg {

[[nodiscard]] ColorInfo toColorInfo(AVColorSpace space, AVColorPrimaries primaries,
                                    AVColorTransferCharacteristic transfer,
                                    AVColorRange range) noexcept;

struct PixelFormatTraits {
    int bit_depth = 8;
    ChromaSubsampling chroma = ChromaSubsampling::Other;
    bool known = false;
};

/// Profondeur et sous-echantillonnage d'un format FFmpeg (formats materiels : known == false).
[[nodiscard]] PixelFormatTraits pixelFormatTraits(AVPixelFormat format) noexcept;

/// Conversion couleur du chemin logiciel (libswscale), explicite (ADR-042) :
///  - YUV -> YUV : meme matrice, plage CONSERVEE (yuvj* et AVCOL_RANGE_JPEG restent en plage
///  pleine,
///    l etiquette de sortie le dit) ;
///  - RVB -> YUV : matrice BT.709, plage limitee ; l etiquette de sortie devient BT.709 limitee.
struct SoftwareColorPlan {
    ColorInfo output;       // etiquette des pixels produits
    int src_colorspace = 0; // SWS_CS_*
    int dst_colorspace = 0; // SWS_CS_*
    bool src_full_range = false;
    bool dst_full_range = false;
    friend constexpr bool operator==(const SoftwareColorPlan&, const SoftwareColorPlan&) = default;
};

[[nodiscard]] SoftwareColorPlan planSoftwareColor(AVPixelFormat source, AVColorSpace space,
                                                  const ColorInfo& tagged) noexcept;

[[nodiscard]] inline Rational toRational(AVRational r) noexcept {
    return Rational{r.num, r.den};
}

} // namespace enh::pipeline::ffmpeg
