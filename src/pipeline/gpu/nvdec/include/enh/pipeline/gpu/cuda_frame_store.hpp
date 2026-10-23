#pragma once

// Magasin d'images GPU pour FfmpegDecoder (docs/gpu/ENH-008.md § 1-2) :
//  - contexte materiel FFmpeg branche sur NOTRE contexte CUDA primaire et NOTRE flux S_dec ;
//  - images NVDEC copiees GPU -> GPU (asynchrone, S_dec) dans un pool de surfaces a taille fixe,
//    le pool FFmpeg est libere aussitot ; back-pressure sur notre pool ;
//  - repli logiciel : conversion CPU dans un tampon epingle (3 tournants), envoi asynchrone.
// Aucune synchronisation CPU dans le chemin NVDEC : chaque surface porte un evenement « pret »,
// et sa reutilisation attend cote GPU l'evenement « dernier usage » du consommateur.

#include "enh/pipeline/ffmpeg/av_raii.hpp"
#include "enh/pipeline/ffmpeg/frame_store.hpp"
#include "enh/pipeline/gpu/cuda_raii.hpp"

#include <cstddef>
#include <memory>

namespace enh::pipeline::gpu {

struct CudaFrameStoreOptions {
    int device = 0;
    std::size_t surfaces = 8;        // pool de surfaces de sortie (= file d'images source, § 3.4)
    std::size_t staging_buffers = 3; // tampons epingles (repli logiciel)
};

namespace detail {
struct CudaShared;
class DevicePool;
class PinnedPool;
} // namespace detail

class CudaFrameStore final : public ffmpeg::IFrameStore {
public:
    [[nodiscard]] static Result<std::shared_ptr<CudaFrameStore>>
    create(const CudaFrameStoreOptions& options = {});

    [[nodiscard]] AVBufferRef* hwDevice() const noexcept override { return hw_device_.get(); }
    [[nodiscard]] Result<FrameBufferPtr> importDevice(const AVFrame& frame, const FrameDesc& desc,
                                                      std::stop_token st) override;
    [[nodiscard]] Result<FrameBufferPtr> acquireStaging(const FrameDesc& desc,
                                                        std::stop_token st) override;
    [[nodiscard]] Result<FrameBufferPtr> commit(FrameBufferPtr staging,
                                                std::stop_token st) override;
    [[nodiscard]] MemoryKind outputMemory() const noexcept override { return MemoryKind::Device; }

    /// Contexte CUDA (CUcontext) de cudart et celui vu par FFmpeg : identiques (un seul contexte).
    [[nodiscard]] void* cudaContext() const noexcept { return context_; }
    [[nodiscard]] void* ffmpegCudaContext() const noexcept;
    /// Flux S_dec (cudaStream_t).
    [[nodiscard]] void* stream() const noexcept;

private:
    struct PrivateTag {};

public:
    explicit CudaFrameStore(PrivateTag /*tag*/) noexcept;
    CudaFrameStore(const CudaFrameStore&) = delete;
    CudaFrameStore& operator=(const CudaFrameStore&) = delete;
    CudaFrameStore(CudaFrameStore&&) = delete;
    CudaFrameStore& operator=(CudaFrameStore&&) = delete;
    ~CudaFrameStore() override;

private:
    Result<std::shared_ptr<detail::DevicePool>> devicePoolFor(const FrameDesc& desc);

    CudaFrameStoreOptions options_;
    void* context_ = nullptr;                    // CUcontext primaire (non possede : cudart)
    std::shared_ptr<detail::CudaShared> shared_; // flux S_dec, partage avec les surfaces
    ffmpeg::BufferRefPtr hw_device_;             // detruit avant le flux (ordre inverse)
    std::shared_ptr<detail::DevicePool> device_pool_;
    std::shared_ptr<detail::PinnedPool> pinned_pool_;
};

} // namespace enh::pipeline::gpu
