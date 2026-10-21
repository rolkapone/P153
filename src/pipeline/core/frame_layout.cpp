#include "enh/pipeline/core/frame_layout.hpp"

#include "enh/pipeline/core/checked_math.hpp"

#include <string>

namespace enh::pipeline {

Result<void> checkDimensions(Size size_px) {
    const auto ok = [](std::uint32_t v) { return v >= kMinDimensionPx && v <= kMaxDimensionPx; };
    if (!ok(size_px.width) || !ok(size_px.height)) {
        return fail(PipelineErrorCode::UnsupportedFormat,
                    "frame dimensions out of bounds: " + std::to_string(size_px.width) + "x" +
                        std::to_string(size_px.height) + " (allowed 1..8192)");
    }
    return {};
}

Result<FrameLayout> computeFrameLayout(Size size_px, PixelFormat format,
                                       std::size_t pitch_alignment) {
    if (auto dims = checkDimensions(size_px); !dims) {
        return enh::Unexpected<PipelineError>(std::move(dims.error()));
    }
    const std::size_t bps = bytesPerSample(format);
    const std::size_t width = size_px.width;
    const std::size_t height = size_px.height;
    const std::size_t chroma_width = (width + 1) / 2; // largeurs impaires : arrondi superieur
    const std::size_t chroma_rows = (height + 1) / 2;

    FrameLayout layout;
    // Plan Y
    auto y_width = checkedMul(width, bps);
    if (!y_width) {
        return enh::Unexpected<PipelineError>(y_width.error());
    }
    auto y_pitch = alignUp(*y_width, pitch_alignment);
    if (!y_pitch) {
        return enh::Unexpected<PipelineError>(y_pitch.error());
    }
    auto y_bytes = checkedMul(*y_pitch, height);
    if (!y_bytes) {
        return enh::Unexpected<PipelineError>(y_bytes.error());
    }
    // Plan UV : deux echantillons (U, V) par position de chrominance
    auto uv_samples = checkedMul(chroma_width, 2);
    if (!uv_samples) {
        return enh::Unexpected<PipelineError>(uv_samples.error());
    }
    auto uv_width = checkedMul(*uv_samples, bps);
    if (!uv_width) {
        return enh::Unexpected<PipelineError>(uv_width.error());
    }
    auto uv_pitch = alignUp(*uv_width, pitch_alignment);
    if (!uv_pitch) {
        return enh::Unexpected<PipelineError>(uv_pitch.error());
    }
    auto uv_bytes = checkedMul(*uv_pitch, chroma_rows);
    if (!uv_bytes) {
        return enh::Unexpected<PipelineError>(uv_bytes.error());
    }
    auto total = checkedAdd(*y_bytes, *uv_bytes);
    if (!total) {
        return enh::Unexpected<PipelineError>(total.error());
    }
    layout.planes[0] = PlaneView{0, *y_pitch, *y_width, height};
    layout.planes[1] =
        PlaneView{static_cast<std::uint64_t>(*y_bytes), *uv_pitch, *uv_width, chroma_rows};
    layout.total_bytes = *total;
    return layout;
}

} // namespace enh::pipeline
