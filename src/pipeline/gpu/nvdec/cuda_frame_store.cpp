#include "enh/pipeline/gpu/cuda_frame_store.hpp"

#include "enh/pipeline/core/checked_math.hpp"
#include "enh/pipeline/core/frame_layout.hpp"

extern "C" {
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_cuda.h>
}

#include <algorithm>
#include <array>
#include <condition_variable>
#include <mutex>
#include <utility>
#include <vector>

namespace enh::pipeline::gpu {

namespace detail {

/// Ressources partagees par le magasin et toutes ses surfaces (vivent jusqu'a la derniere).
struct CudaShared {
    int device = 0;
    CudaStream stream;
};

/// File de numeros de cases libres, attente interruptible (back-pressure).
class SlotQueue {
public:
    explicit SlotQueue(std::size_t n) {
        free_.reserve(n);
        for (std::size_t i = n; i > 0; --i) {
            free_.push_back(i - 1);
        }
    }
    Result<std::size_t> acquire(const std::stop_token& st) {
        std::unique_lock lock(mutex_);
        if (!cv_.wait(lock, st, [this] { return !free_.empty(); })) {
            return fail(PipelineErrorCode::Cancelled, "cancelled while waiting for a GPU surface");
        }
        const std::size_t i = free_.back();
        free_.pop_back();
        return i;
    }
    void release(std::size_t i) noexcept {
        {
            const std::scoped_lock lock(mutex_);
            free_.push_back(i);
        }
        cv_.notify_one();
    }

private:
    std::mutex mutex_;
    std::condition_variable_any cv_;
    std::vector<std::size_t> free_;
};

/// Pool de surfaces GPU NV12 / P010 (une allocation a pas par surface : Y puis UV).
class DevicePool : public std::enable_shared_from_this<DevicePool> {
public:
    struct Slot {
        DeviceMemory memory;
        CudaEvent ready;
        CudaEvent released;
        bool released_recorded = false;
        std::array<PlaneView, kMaxPlanes> planes{};
    };

    DevicePool(std::shared_ptr<CudaShared> shared, const FrameDesc& desc, std::size_t n)
        : shared_(std::move(shared)), desc_(desc), slots_(n), queue_(n) {}

    static Result<std::shared_ptr<DevicePool>> create(std::shared_ptr<CudaShared> shared,
                                                      const FrameDesc& desc, std::size_t n) {
        auto layout = computeFrameLayout(desc.size_px, desc.format, 1);
        if (!layout) {
            return enh::Unexpected<PipelineError>(layout.error());
        }
        const PlaneView& y = layout->planes[0];
        const PlaneView& uv = layout->planes[1];
        auto rows = checkedAdd(y.rows, uv.rows);
        if (!rows) {
            return enh::Unexpected<PipelineError>(rows.error());
        }
        auto pool = std::make_shared<DevicePool>(std::move(shared), desc, n);
        for (Slot& s : pool->slots_) {
            auto mem = DeviceMemory::allocPitch(std::max(y.width_bytes, uv.width_bytes), *rows);
            if (!mem) {
                return enh::Unexpected<PipelineError>(std::move(mem.error()));
            }
            s.memory = std::move(*mem);
            auto uv_offset = checkedMul(s.memory.pitch(), y.rows);
            if (!uv_offset) {
                return enh::Unexpected<PipelineError>(uv_offset.error());
            }
            s.planes[0] = PlaneView{s.memory.address(), s.memory.pitch(), y.width_bytes, y.rows};
            s.planes[1] = PlaneView{s.memory.address() + *uv_offset, s.memory.pitch(),
                                    uv.width_bytes, uv.rows};
            auto ready = makeEvent();
            auto released = makeEvent();
            if (!ready || !released) {
                return enh::Unexpected<PipelineError>(!ready ? ready.error() : released.error());
            }
            s.ready = std::move(*ready);
            s.released = std::move(*released);
        }
        return pool;
    }

    [[nodiscard]] const FrameDesc& desc() const noexcept { return desc_; }
    [[nodiscard]] CudaShared& shared() const noexcept { return *shared_; }
    Slot& slot(std::size_t i) noexcept { return slots_[i]; }
    Result<std::size_t> acquire(const std::stop_token& st) { return queue_.acquire(st); }
    void release(std::size_t i) noexcept { queue_.release(i); }

private:
    std::shared_ptr<CudaShared> shared_;
    FrameDesc desc_;
    std::vector<Slot> slots_;
    SlotQueue queue_;
};

/// Surface GPU remise au pool par son destructeur (RAII).
class DeviceFrameBuffer final : public FrameBuffer {
public:
    DeviceFrameBuffer(std::shared_ptr<DevicePool> pool, std::size_t slot) noexcept
        : pool_(std::move(pool)), slot_(slot) {}
    DeviceFrameBuffer(const DeviceFrameBuffer&) = delete;
    DeviceFrameBuffer& operator=(const DeviceFrameBuffer&) = delete;
    DeviceFrameBuffer(DeviceFrameBuffer&&) = delete;
    DeviceFrameBuffer& operator=(DeviceFrameBuffer&&) = delete;
    ~DeviceFrameBuffer() override {
        DevicePool::Slot& s = pool_->slot(slot_);
        if (!last_use_recorded_) {
            // Sans declaration du consommateur : reutilisation apres tout le travail de S_dec.
            s.released_recorded =
                cudaEventRecord(s.released.get(), pool_->shared().stream.get()) == cudaSuccess;
        }
        pool_->release(slot_);
    }
    [[nodiscard]] MemoryKind memory() const noexcept override { return MemoryKind::Device; }
    [[nodiscard]] std::span<const PlaneView> planes() const noexcept override {
        return pool_->slot(slot_).planes;
    }
    [[nodiscard]] void* readyEvent() const noexcept override {
        return pool_->slot(slot_).ready.get();
    }
    void recordLastUse(void* cuda_stream) noexcept override {
        DevicePool::Slot& s = pool_->slot(slot_);
        s.released_recorded =
            cudaEventRecord(s.released.get(), static_cast<cudaStream_t>(cuda_stream)) ==
            cudaSuccess;
        last_use_recorded_ = s.released_recorded;
    }

private:
    std::shared_ptr<DevicePool> pool_;
    std::size_t slot_;
    bool last_use_recorded_ = false;
};

/// Tampons epingles tournants (repli logiciel).
class PinnedPool : public std::enable_shared_from_this<PinnedPool> {
public:
    struct Slot {
        PinnedMemory memory;
        CudaEvent uploaded;
        bool pending = false;
        std::array<PlaneView, kMaxPlanes> planes{};
    };
    static constexpr std::size_t kAlignment = 64;

    PinnedPool(const FrameDesc& desc, std::size_t n) : desc_(desc), slots_(n), queue_(n) {}

    static Result<std::shared_ptr<PinnedPool>> create(const FrameDesc& desc, std::size_t n) {
        auto layout = computeFrameLayout(desc.size_px, desc.format, kAlignment);
        if (!layout) {
            return enh::Unexpected<PipelineError>(layout.error());
        }
        auto pool = std::make_shared<PinnedPool>(desc, n);
        for (Slot& s : pool->slots_) {
            auto mem = PinnedMemory::alloc(layout->total_bytes);
            if (!mem) {
                return enh::Unexpected<PipelineError>(std::move(mem.error()));
            }
            s.memory = std::move(*mem);
            auto ev = makeEvent();
            if (!ev) {
                return enh::Unexpected<PipelineError>(std::move(ev.error()));
            }
            s.uploaded = std::move(*ev);
            const auto base = reinterpret_cast<std::uint64_t>(s.memory.data());
            for (std::size_t i = 0; i < kMaxPlanes; ++i) {
                s.planes[i] = layout->planes[i];
                s.planes[i].address += base;
            }
        }
        return pool;
    }

    [[nodiscard]] const FrameDesc& desc() const noexcept { return desc_; }
    Slot& slot(std::size_t i) noexcept { return slots_[i]; }
    Result<std::size_t> acquire(const std::stop_token& st) { return queue_.acquire(st); }
    void release(std::size_t i) noexcept { queue_.release(i); }

private:
    FrameDesc desc_;
    std::vector<Slot> slots_;
    SlotQueue queue_;
};

class PinnedFrameBuffer final : public FrameBuffer {
public:
    PinnedFrameBuffer(std::shared_ptr<PinnedPool> pool, std::size_t slot) noexcept
        : pool_(std::move(pool)), slot_(slot) {}
    PinnedFrameBuffer(const PinnedFrameBuffer&) = delete;
    PinnedFrameBuffer& operator=(const PinnedFrameBuffer&) = delete;
    PinnedFrameBuffer(PinnedFrameBuffer&&) = delete;
    PinnedFrameBuffer& operator=(PinnedFrameBuffer&&) = delete;
    ~PinnedFrameBuffer() override { pool_->release(slot_); }
    [[nodiscard]] MemoryKind memory() const noexcept override { return MemoryKind::Host; }
    [[nodiscard]] std::span<const PlaneView> planes() const noexcept override {
        return pool_->slot(slot_).planes;
    }
    [[nodiscard]] PinnedPool::Slot& slot() const noexcept { return pool_->slot(slot_); }
    [[nodiscard]] const FrameDesc& desc() const noexcept { return pool_->desc(); }

private:
    std::shared_ptr<PinnedPool> pool_;
    std::size_t slot_;
};

} // namespace detail

using detail::DeviceFrameBuffer;
using detail::DevicePool;
using detail::PinnedFrameBuffer;
using detail::PinnedPool;

namespace {

/// Copie 2D asynchrone des deux plans vers une surface du pool, puis evenement « pret ».
Result<FrameBufferPtr> copyIntoSurface(const std::shared_ptr<DevicePool>& pool,
                                       const std::array<const void*, 2>& src,
                                       const std::array<std::size_t, 2>& src_pitch,
                                       cudaMemcpyKind kind, const std::stop_token& st) {
    auto idx = pool->acquire(st);
    if (!idx) {
        return enh::Unexpected<PipelineError>(std::move(idx.error()));
    }
    // RAII des maintenant : en cas d'erreur ci-dessous, la surface revient au pool.
    auto buffer = std::make_unique<DeviceFrameBuffer>(pool, *idx);
    DevicePool::Slot& s = pool->slot(*idx);
    cudaStream_t stream = pool->shared().stream.get();
    if (s.released_recorded) {
        if (auto r = checkCuda(cudaStreamWaitEvent(stream, s.released.get(), 0),
                               "cudaStreamWaitEvent(released)");
            !r) {
            return enh::Unexpected<PipelineError>(std::move(r.error()));
        }
    }
    for (std::size_t i = 0; i < 2; ++i) {
        const PlaneView& dst = s.planes[i];
        if (auto r = checkCuda(cudaMemcpy2DAsync(reinterpret_cast<void*>(dst.address),
                                                 dst.pitch_bytes, src[i], src_pitch[i],
                                                 dst.width_bytes, dst.rows, kind, stream),
                               "cudaMemcpy2DAsync");
            !r) {
            return enh::Unexpected<PipelineError>(std::move(r.error()));
        }
    }
    if (auto r = checkCuda(cudaEventRecord(s.ready.get(), stream), "cudaEventRecord(ready)"); !r) {
        return enh::Unexpected<PipelineError>(std::move(r.error()));
    }
    return FrameBufferPtr{std::move(buffer)};
}

} // namespace

CudaFrameStore::CudaFrameStore(PrivateTag /*tag*/) noexcept {}

CudaFrameStore::~CudaFrameStore() {
    // Les copies en vol sur S_dec se terminent avant la liberation des pools FFmpeg.
    if (shared_ && shared_->stream) {
        cudaStreamSynchronize(shared_->stream.get());
    }
}

Result<std::shared_ptr<CudaFrameStore>>
CudaFrameStore::create(const CudaFrameStoreOptions& options) {
    if (options.surfaces == 0 || options.staging_buffers == 0) {
        return fail(PipelineErrorCode::InvalidParams, "pool sizes must be > 0");
    }
    auto ctx = initPrimaryContext(options.device);
    if (!ctx) {
        return enh::Unexpected<PipelineError>(std::move(ctx.error()));
    }
    auto self = std::make_shared<CudaFrameStore>(PrivateTag{});
    self->options_ = options;
    self->context_ = *ctx;
    auto stream = makeStream();
    if (!stream) {
        return enh::Unexpected<PipelineError>(std::move(stream.error()));
    }
    self->shared_ = std::make_shared<detail::CudaShared>();
    self->shared_->device = options.device;
    self->shared_->stream = std::move(*stream);

    // Contexte materiel FFmpeg sur NOTRE contexte et NOTRE flux (libavutil/hwcontext_cuda.h) :
    // FFmpeg ne cree pas de second contexte et ne libere pas le notre (non alloue par lui).
    ffmpeg::BufferRefPtr ref(av_hwdevice_ctx_alloc(AV_HWDEVICE_TYPE_CUDA));
    if (!ref) {
        return fail(PipelineErrorCode::Internal, "av_hwdevice_ctx_alloc(CUDA) failed");
    }
    auto* device_ctx = reinterpret_cast<AVHWDeviceContext*>(ref->data);
    auto* cuda_ctx = static_cast<AVCUDADeviceContext*>(device_ctx->hwctx);
    cuda_ctx->cuda_ctx = static_cast<CUcontext>(self->context_);
    cuda_ctx->stream = self->shared_->stream.get();
    if (const int r = av_hwdevice_ctx_init(ref.get()); r < 0) {
        return fail(PipelineErrorCode::ComponentMissing,
                    "av_hwdevice_ctx_init(CUDA) failed (NVDEC unavailable)", r);
    }
    self->hw_device_ = std::move(ref);
    return self;
}

void* CudaFrameStore::ffmpegCudaContext() const noexcept {
    if (!hw_device_) {
        return nullptr;
    }
    const auto* device_ctx = reinterpret_cast<const AVHWDeviceContext*>(hw_device_->data);
    return static_cast<const AVCUDADeviceContext*>(device_ctx->hwctx)->cuda_ctx;
}

void* CudaFrameStore::stream() const noexcept {
    return shared_ ? shared_->stream.get() : nullptr;
}

Result<std::shared_ptr<DevicePool>> CudaFrameStore::devicePoolFor(const FrameDesc& desc) {
    if (!device_pool_ || device_pool_->desc().size_px != desc.size_px ||
        device_pool_->desc().format != desc.format) {
        // Changement de format en cours de flux : nouveau pool ; l'ancien vit tant que des
        // surfaces en circulation le referencent.
        auto pool = DevicePool::create(shared_, desc, options_.surfaces);
        if (!pool) {
            return pool;
        }
        device_pool_ = std::move(*pool);
    }
    return device_pool_;
}

Result<FrameBufferPtr> CudaFrameStore::importDevice(const AVFrame& frame, const FrameDesc& desc,
                                                    std::stop_token st) {
    if (auto r = checkCuda(cudaSetDevice(options_.device), "cudaSetDevice"); !r) {
        return enh::Unexpected<PipelineError>(std::move(r.error()));
    }
    auto pool = devicePoolFor(desc);
    if (!pool) {
        return enh::Unexpected<PipelineError>(std::move(pool.error()));
    }
    if (frame.linesize[0] <= 0 || frame.linesize[1] <= 0) {
        return fail(PipelineErrorCode::Internal, "unexpected CUDA frame layout");
    }
    const std::array<const void*, 2> src{frame.data[0], frame.data[1]};
    const std::array<std::size_t, 2> pitch{static_cast<std::size_t>(frame.linesize[0]),
                                           static_cast<std::size_t>(frame.linesize[1])};
    // L image NVDEC peut etre rendue a FFmpeg aussitot : sa reutilisation passe par le meme flux.
    return copyIntoSurface(*pool, src, pitch, cudaMemcpyDeviceToDevice, st);
}

Result<FrameBufferPtr> CudaFrameStore::acquireStaging(const FrameDesc& desc, std::stop_token st) {
    if (!pinned_pool_ || pinned_pool_->desc().size_px != desc.size_px ||
        pinned_pool_->desc().format != desc.format) {
        auto pool = PinnedPool::create(desc, options_.staging_buffers);
        if (!pool) {
            return enh::Unexpected<PipelineError>(std::move(pool.error()));
        }
        pinned_pool_ = std::move(*pool);
    }
    auto idx = pinned_pool_->acquire(st);
    if (!idx) {
        return enh::Unexpected<PipelineError>(std::move(idx.error()));
    }
    auto buffer = std::make_unique<PinnedFrameBuffer>(pinned_pool_, *idx);
    PinnedPool::Slot& s = pinned_pool_->slot(*idx);
    if (s.pending) {
        // Repli logiciel seulement : on reecrit un tampon dont l'envoi precedent doit etre fini.
        if (auto r = checkCuda(cudaEventSynchronize(s.uploaded.get()), "cudaEventSynchronize");
            !r) {
            return enh::Unexpected<PipelineError>(std::move(r.error()));
        }
        s.pending = false;
    }
    return FrameBufferPtr{std::move(buffer)};
}

Result<FrameBufferPtr> CudaFrameStore::commit(FrameBufferPtr staging, std::stop_token st) {
    auto* pinned = dynamic_cast<PinnedFrameBuffer*>(staging.get());
    if (pinned == nullptr) {
        return fail(PipelineErrorCode::Internal, "commit expects a buffer from acquireStaging");
    }
    if (auto r = checkCuda(cudaSetDevice(options_.device), "cudaSetDevice"); !r) {
        return enh::Unexpected<PipelineError>(std::move(r.error()));
    }
    auto pool = devicePoolFor(pinned->desc());
    if (!pool) {
        return enh::Unexpected<PipelineError>(std::move(pool.error()));
    }
    const auto planes = pinned->planes();
    const std::array<const void*, 2> src{reinterpret_cast<const void*>(planes[0].address),
                                         reinterpret_cast<const void*>(planes[1].address)};
    const std::array<std::size_t, 2> pitch{planes[0].pitch_bytes, planes[1].pitch_bytes};
    auto out = copyIntoSurface(*pool, src, pitch, cudaMemcpyHostToDevice, st);
    if (out) {
        PinnedPool::Slot& s = pinned->slot();
        if (auto r = checkCuda(cudaEventRecord(s.uploaded.get(), shared_->stream.get()),
                               "cudaEventRecord(uploaded)");
            !r) {
            return enh::Unexpected<PipelineError>(std::move(r.error()));
        }
        s.pending = true;
    }
    return out; // `staging` revient a son pool ici
}

} // namespace enh::pipeline::gpu
