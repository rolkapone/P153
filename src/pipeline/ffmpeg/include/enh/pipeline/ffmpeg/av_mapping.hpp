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

[[nodiscard]] inline Rational toRational(AVRational r) noexcept {
    return Rational{r.num, r.den};
}

} // namespace enh::pipeline::ffmpeg
