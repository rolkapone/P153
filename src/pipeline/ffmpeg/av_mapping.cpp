#include "enh/pipeline/ffmpeg/av_mapping.hpp"

extern "C" {
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
}

namespace enh::pipeline::ffmpeg {

ColorInfo toColorInfo(AVColorSpace space, AVColorPrimaries primaries,
                      AVColorTransferCharacteristic transfer, AVColorRange range) noexcept {
    ColorInfo c;
    switch (space) {
    case AVCOL_SPC_BT709:
        c.matrix = ColorMatrix::Bt709;
        break;
    case AVCOL_SPC_BT470BG:
    case AVCOL_SPC_SMPTE170M:
        c.matrix = ColorMatrix::Bt601;
        break;
    case AVCOL_SPC_BT2020_NCL:
        c.matrix = ColorMatrix::Bt2020Ncl;
        break;
    case AVCOL_SPC_UNSPECIFIED:
        c.matrix = ColorMatrix::Unspecified;
        break;
    default:
        c.matrix = ColorMatrix::Other;
        break;
    }
    switch (primaries) {
    case AVCOL_PRI_BT709:
        c.primaries = ColorPrimaries::Bt709;
        break;
    case AVCOL_PRI_BT470BG:
    case AVCOL_PRI_SMPTE170M:
        c.primaries = ColorPrimaries::Bt601;
        break;
    case AVCOL_PRI_BT2020:
        c.primaries = ColorPrimaries::Bt2020;
        break;
    case AVCOL_PRI_UNSPECIFIED:
        c.primaries = ColorPrimaries::Unspecified;
        break;
    default:
        c.primaries = ColorPrimaries::Other;
        break;
    }
    switch (transfer) {
    case AVCOL_TRC_BT709:
    case AVCOL_TRC_SMPTE170M:
    case AVCOL_TRC_BT2020_10:
    case AVCOL_TRC_BT2020_12:
        c.transfer = ColorTransfer::Bt709;
        break;
    case AVCOL_TRC_SMPTE2084:
        c.transfer = ColorTransfer::Pq;
        break;
    case AVCOL_TRC_ARIB_STD_B67:
        c.transfer = ColorTransfer::Hlg;
        break;
    case AVCOL_TRC_UNSPECIFIED:
        c.transfer = ColorTransfer::Unspecified;
        break;
    default:
        c.transfer = ColorTransfer::Other;
        break;
    }
    switch (range) {
    case AVCOL_RANGE_MPEG:
        c.range = ColorRange::Limited;
        break;
    case AVCOL_RANGE_JPEG:
        c.range = ColorRange::Full;
        break;
    default:
        c.range = ColorRange::Unspecified;
        break;
    }
    return c;
}

PixelFormatTraits pixelFormatTraits(AVPixelFormat format) noexcept {
    PixelFormatTraits t;
    const AVPixFmtDescriptor* d = av_pix_fmt_desc_get(format);
    if (d == nullptr || (d->flags & AV_PIX_FMT_FLAG_HWACCEL) != 0 || d->nb_components == 0) {
        return t;
    }
    t.known = true;
    t.bit_depth = d->comp[0].depth;
    if ((d->flags & AV_PIX_FMT_FLAG_RGB) != 0) {
        t.chroma = ChromaSubsampling::Other;
    } else if (d->nb_components <= 2) {
        t.chroma = ChromaSubsampling::Gray;
    } else if (d->log2_chroma_w == 1 && d->log2_chroma_h == 1) {
        t.chroma = ChromaSubsampling::Yuv420;
    } else if (d->log2_chroma_w == 1 && d->log2_chroma_h == 0) {
        t.chroma = ChromaSubsampling::Yuv422;
    } else if (d->log2_chroma_w == 0 && d->log2_chroma_h == 0) {
        t.chroma = ChromaSubsampling::Yuv444;
    }
    return t;
}

} // namespace enh::pipeline::ffmpeg

namespace enh::pipeline::ffmpeg {

namespace {

bool isJpegRangeFormat(AVPixelFormat f) noexcept {
    // Formats « J » (plage pleine) encore produits par les decodeurs MJPEG.
    return f == AV_PIX_FMT_YUVJ420P || f == AV_PIX_FMT_YUVJ422P || f == AV_PIX_FMT_YUVJ444P ||
           f == AV_PIX_FMT_YUVJ440P || f == AV_PIX_FMT_YUVJ411P;
}

int swsColorspace(AVColorSpace space) noexcept {
    switch (space) {
    case AVCOL_SPC_BT709:
        return SWS_CS_ITU709;
    case AVCOL_SPC_BT2020_NCL:
    case AVCOL_SPC_BT2020_CL:
        return SWS_CS_BT2020;
    case AVCOL_SPC_SMPTE240M:
        return SWS_CS_SMPTE240M;
    case AVCOL_SPC_FCC:
        return SWS_CS_FCC;
    default:
        return SWS_CS_ITU601; // defaut de libswscale
    }
}

} // namespace

SoftwareColorPlan planSoftwareColor(AVPixelFormat source, AVColorSpace space,
                                    const ColorInfo& tagged) noexcept {
    SoftwareColorPlan plan;
    plan.output = tagged;
    const AVPixFmtDescriptor* d = av_pix_fmt_desc_get(source);
    const bool rgb = d != nullptr && (d->flags & AV_PIX_FMT_FLAG_RGB) != 0;
    if (rgb) {
        plan.src_colorspace = SWS_CS_ITU709;
        plan.dst_colorspace = SWS_CS_ITU709;
        plan.src_full_range = true; // le RVB est toujours en plage pleine
        plan.dst_full_range = false;
        plan.output.matrix = ColorMatrix::Bt709;
        plan.output.range = ColorRange::Limited;
        if (plan.output.primaries == ColorPrimaries::Unspecified) {
            plan.output.primaries = ColorPrimaries::Bt709;
        }
        return plan;
    }
    const bool full = isJpegRangeFormat(source) || tagged.range == ColorRange::Full;
    plan.src_colorspace = swsColorspace(space);
    plan.dst_colorspace = plan.src_colorspace; // YUV -> YUV : aucune conversion de matrice
    plan.src_full_range = full;
    plan.dst_full_range = full;
    if (full) {
        plan.output.range = ColorRange::Full;
    }
    return plan;
}

} // namespace enh::pipeline::ffmpeg
