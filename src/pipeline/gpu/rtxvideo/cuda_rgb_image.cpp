#include "enh/pipeline/gpu/cuda_rgb_image.hpp"

#include "enh/pipeline/gpu/cuda_raii.hpp"

#include <utility>

namespace enh::pipeline::gpu {

namespace {

constexpr std::uint32_t kMaxSidePx = 16384; // borne SEC-13 (pas d'image absurde)
constexpr std::size_t kBytesPerPixel = 4;

cudaChannelFormatDesc channelDesc(RgbFormat format) noexcept {
    if (format == RgbFormat::Abgr10) {
        return cudaCreateChannelDesc(10, 10, 10, 2, cudaChannelFormatKindUnsignedNormalized1010102);
    }
    return cudaCreateChannelDesc(8, 8, 8, 8, cudaChannelFormatKindUnsigned);
}

} // namespace

Result<CudaRgbImage> CudaRgbImage::create(Size size_px, RgbFormat format) {
    if (size_px.width == 0 || size_px.height == 0 || size_px.width > kMaxSidePx ||
        size_px.height > kMaxSidePx) {
        return fail(PipelineErrorCode::InvalidParams, "CudaRgbImage: bad size");
    }
    CudaRgbImage img;
    img.size_ = size_px;
    img.format_ = format;
    const cudaChannelFormatDesc desc = channelDesc(format);
    if (auto r = checkCuda(cudaMallocArray(&img.array_, &desc, size_px.width, size_px.height,
                                           cudaArraySurfaceLoadStore),
                           "cudaMallocArray");
        !r) {
        PipelineError e = std::move(r.error());
        e.required_mib = static_cast<std::uint32_t>(
            (img.pixelCount() * kBytesPerPixel + (std::size_t{1} << 20) - 1) >> 20);
        return enh::Unexpected<PipelineError>(std::move(e));
    }
    cudaResourceDesc res{};
    res.resType = cudaResourceTypeArray;
    res.res.array.array = img.array_;

    cudaTextureDesc tex{};
    tex.addressMode[0] = cudaAddressModeClamp;
    tex.addressMode[1] = cudaAddressModeClamp;
    tex.addressMode[2] = cudaAddressModeClamp;
    tex.filterMode = cudaFilterModeLinear;
    tex.normalizedCoords = 1;
    // 8 bits : entiers lus en flottants normalises ; 10 bits : format deja normalise.
    tex.readMode =
        format == RgbFormat::Argb8 ? cudaReadModeNormalizedFloat : cudaReadModeElementType;
    if (auto r = checkCuda(cudaCreateTextureObject(&img.texture_, &res, &tex, nullptr),
                           "cudaCreateTextureObject");
        !r) {
        return enh::Unexpected<PipelineError>(std::move(r.error()));
    }
    if (auto r = checkCuda(cudaCreateSurfaceObject(&img.surface_, &res), "cudaCreateSurfaceObject");
        !r) {
        return enh::Unexpected<PipelineError>(std::move(r.error()));
    }
    return img;
}

CudaRgbImage::CudaRgbImage(CudaRgbImage&& other) noexcept
    : array_(std::exchange(other.array_, nullptr)), texture_(std::exchange(other.texture_, 0)),
      surface_(std::exchange(other.surface_, 0)), size_(std::exchange(other.size_, Size{})),
      format_(other.format_) {}

CudaRgbImage& CudaRgbImage::operator=(CudaRgbImage&& other) noexcept {
    if (this != &other) {
        reset();
        array_ = std::exchange(other.array_, nullptr);
        texture_ = std::exchange(other.texture_, 0);
        surface_ = std::exchange(other.surface_, 0);
        size_ = std::exchange(other.size_, Size{});
        format_ = other.format_;
    }
    return *this;
}

CudaRgbImage::~CudaRgbImage() {
    reset();
}

void CudaRgbImage::reset() noexcept {
    if (surface_ != 0) {
        cudaDestroySurfaceObject(std::exchange(surface_, 0));
    }
    if (texture_ != 0) {
        cudaDestroyTextureObject(std::exchange(texture_, 0));
    }
    if (array_ != nullptr) {
        cudaFreeArray(std::exchange(array_, nullptr));
    }
}

Result<void> CudaRgbImage::upload(std::span<const std::uint32_t> pixels, cudaStream_t stream) {
    if (array_ == nullptr || pixels.size() != pixelCount()) {
        return fail(PipelineErrorCode::InvalidParams, "CudaRgbImage::upload: size mismatch");
    }
    const std::size_t row = std::size_t{size_.width} * kBytesPerPixel;
    return checkCuda(cudaMemcpy2DToArrayAsync(array_, 0, 0, pixels.data(), row, row, size_.height,
                                              cudaMemcpyHostToDevice, stream),
                     "cudaMemcpy2DToArrayAsync");
}

Result<void> CudaRgbImage::download(std::span<std::uint32_t> pixels, cudaStream_t stream) const {
    if (array_ == nullptr || pixels.size() != pixelCount()) {
        return fail(PipelineErrorCode::InvalidParams, "CudaRgbImage::download: size mismatch");
    }
    const std::size_t row = std::size_t{size_.width} * kBytesPerPixel;
    return checkCuda(cudaMemcpy2DFromArrayAsync(pixels.data(), row, array_, 0, 0, row, size_.height,
                                                cudaMemcpyDeviceToHost, stream),
                     "cudaMemcpy2DFromArrayAsync");
}

} // namespace enh::pipeline::gpu
