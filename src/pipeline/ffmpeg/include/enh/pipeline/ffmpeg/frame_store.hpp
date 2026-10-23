#pragma once

// Destination des images decodees par FfmpegDecoder. Deux implementations :
//  - HostFrameStore (ici, sans CUDA) : images en memoire CPU (CI, outils) ;
//  - CudaFrameStore (src/pipeline/gpu/nvdec) : surfaces GPU, NVDEC + envoi epingle en repli.

#include "enh/pipeline/core/host_frame_pool.hpp"
#include "enh/pipeline/stage/frame.hpp"

#include <cstddef>
#include <memory>
#include <stop_token>

struct AVBufferRef;
struct AVFrame;

namespace enh::pipeline::ffmpeg {

class IFrameStore {
public:
    IFrameStore() = default;
    IFrameStore(const IFrameStore&) = delete;
    IFrameStore& operator=(const IFrameStore&) = delete;
    IFrameStore(IFrameStore&&) = delete;
    IFrameStore& operator=(IFrameStore&&) = delete;
    virtual ~IFrameStore() = default;

    /// Contexte materiel FFmpeg (AV_HWDEVICE_TYPE_CUDA) ; nullptr = pas de NVDEC.
    [[nodiscard]] virtual AVBufferRef* hwDevice() const noexcept { return nullptr; }

    /// Copie GPU -> GPU d'une image NVDEC (AV_PIX_FMT_CUDA, sw_format NV12 / P010 / P016) dans
    /// une surface du pool. Asynchrone : la surface porte un evenement « pret ».
    [[nodiscard]] virtual Result<FrameBufferPtr>
    importDevice(const AVFrame& frame, const FrameDesc& desc, std::stop_token st);

    /// Tampon CPU inscriptible au format `desc` (le decodeur y ecrit les pixels convertis).
    [[nodiscard]] virtual Result<FrameBufferPtr> acquireStaging(const FrameDesc& desc,
                                                                std::stop_token st) = 0;

    /// Image finale a partir d'un tampon rempli : le meme (CPU) ou une surface GPU (envoi
    /// asynchrone depuis la memoire epinglee).
    [[nodiscard]] virtual Result<FrameBufferPtr> commit(FrameBufferPtr staging,
                                                        std::stop_token st) = 0;

    [[nodiscard]] virtual MemoryKind outputMemory() const noexcept = 0;
};

/// Images en memoire CPU, pool borne (recree si le format change en cours de flux).
class HostFrameStore final : public IFrameStore {
public:
    explicit HostFrameStore(std::size_t capacity = 8) noexcept : capacity_(capacity) {}

    [[nodiscard]] Result<FrameBufferPtr> acquireStaging(const FrameDesc& desc,
                                                        std::stop_token st) override;
    [[nodiscard]] Result<FrameBufferPtr> commit(FrameBufferPtr staging,
                                                std::stop_token st) override;
    [[nodiscard]] MemoryKind outputMemory() const noexcept override { return MemoryKind::Host; }

private:
    std::size_t capacity_;
    std::shared_ptr<HostFramePool> pool_;
};

} // namespace enh::pipeline::ffmpeg
