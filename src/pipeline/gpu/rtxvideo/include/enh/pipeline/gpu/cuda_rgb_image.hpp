#pragma once

// Image RGB sur le GPU pour le RTX Video SDK (ENH-040) : tableau CUDA (surface load/store) +
// objet texture (lecture normalisee, filtrage lineaire, coordonnees normalisees : description
// de samples/RTX_Video_API/rtx_video_api_cuda_impl.cpp) + objet surface (ecriture).
// RAII : cudaDestroySurfaceObject / cudaDestroyTextureObject / cudaFreeArray.

#include "enh/pipeline/error.hpp"
#include "enh/pipeline/stage/upscaler.hpp"
#include "enh/pipeline/types.hpp"

#include <cuda_runtime_api.h>

#include <cstdint>
#include <span>

namespace enh::pipeline::gpu {

class CudaRgbImage {
public:
    [[nodiscard]] static Result<CudaRgbImage> create(Size size_px, RgbFormat format);

    CudaRgbImage() = default;
    CudaRgbImage(CudaRgbImage&& other) noexcept;
    CudaRgbImage& operator=(CudaRgbImage&& other) noexcept;
    CudaRgbImage(const CudaRgbImage&) = delete;
    CudaRgbImage& operator=(const CudaRgbImage&) = delete;
    ~CudaRgbImage();

    /// Copie hote -> image (4 octets par pixel, lignes contigues), asynchrone sur `stream`.
    /// `pixels` doit rester valide jusqu'a la fin de la copie (memoire epinglee conseillee).
    [[nodiscard]] Result<void> upload(std::span<const std::uint32_t> pixels, cudaStream_t stream);
    /// Copie image -> hote, asynchrone sur `stream`.
    [[nodiscard]] Result<void> download(std::span<std::uint32_t> pixels, cudaStream_t stream) const;

    [[nodiscard]] std::uint64_t texture() const noexcept { return texture_; }
    [[nodiscard]] std::uint64_t surface() const noexcept { return surface_; }
    [[nodiscard]] Size size() const noexcept { return size_; }
    [[nodiscard]] RgbFormat format() const noexcept { return format_; }
    [[nodiscard]] std::size_t pixelCount() const noexcept {
        return static_cast<std::size_t>(size_.width) * size_.height;
    }

private:
    void reset() noexcept;

    cudaArray_t array_ = nullptr;
    cudaTextureObject_t texture_ = 0;
    cudaSurfaceObject_t surface_ = 0;
    Size size_;
    RgbFormat format_ = RgbFormat::Argb8;
};

/// Requete de super-resolution construite A PARTIR des images reelles (tailles et poignees
/// coherentes par construction : pas de rectangle plus grand que l image, donc pas d ecriture hors
/// bornes).
[[nodiscard]] inline UpscaleRequest makeUpscaleRequest(const CudaRgbImage& in,
                                                       const CudaRgbImage& out,
                                                       UpscaleQuality quality) noexcept {
    UpscaleRequest r;
    r.input_texture = in.texture();
    r.input_size_px = in.size();
    r.output_surface = out.surface();
    r.output_size_px = out.size();
    r.quality = quality;
    return r;
}

} // namespace enh::pipeline::gpu
