#include "enh/pipeline/core/host_frame_pool.hpp"

#include "enh/pipeline/core/checked_math.hpp"

#include <array>
#include <memory>
#include <span>
#include <utility>

namespace enh::pipeline {

class HostFramePool::Buffer final : public FrameBuffer {
public:
    Buffer(std::shared_ptr<HostFramePool> pool, std::size_t slot) noexcept
        : pool_(std::move(pool)), slot_(slot) {
        const auto base = reinterpret_cast<std::uint64_t>(pool_->slots_[slot_].base);
        for (std::size_t i = 0; i < kMaxPlanes; ++i) {
            planes_[i] = pool_->layout_.planes[i];
            planes_[i].address += base;
        }
    }
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    Buffer(Buffer&&) = delete;
    Buffer& operator=(Buffer&&) = delete;
    ~Buffer() override { pool_->release(slot_); }

    [[nodiscard]] MemoryKind memory() const noexcept override { return MemoryKind::Host; }
    [[nodiscard]] std::span<const PlaneView> planes() const noexcept override { return planes_; }

private:
    std::shared_ptr<HostFramePool> pool_;
    std::size_t slot_;
    std::array<PlaneView, kMaxPlanes> planes_{};
};

HostFramePool::HostFramePool(PrivateTag /*tag*/, const FrameDesc& desc, const FrameLayout& layout)
    : desc_(desc), layout_(layout) {}

Result<std::shared_ptr<HostFramePool>> HostFramePool::create(const FrameDesc& desc,
                                                             std::size_t capacity) {
    if (capacity == 0) {
        return fail(PipelineErrorCode::InvalidParams, "host frame pool capacity must be > 0");
    }
    auto layout = computeFrameLayout(desc.size_px, desc.format, kPitchAlignment);
    if (!layout) {
        return enh::Unexpected<PipelineError>(layout.error());
    }
    auto bytes = checkedAdd(layout->total_bytes, kPitchAlignment);
    if (!bytes) {
        return enh::Unexpected<PipelineError>(bytes.error());
    }
    auto pool = std::make_shared<HostFramePool>(PrivateTag{}, desc, *layout);
    pool->slots_.resize(capacity);
    pool->free_.reserve(capacity);
    for (std::size_t i = 0; i < capacity; ++i) {
        Slot& s = pool->slots_[i];
        s.storage.resize(*bytes);
        void* p = s.storage.data();
        std::size_t space = s.storage.size();
        s.base =
            static_cast<std::byte*>(std::align(kPitchAlignment, layout->total_bytes, p, space));
        pool->free_.push_back(capacity - 1 - i);
    }
    return pool;
}

Result<FrameBufferPtr> HostFramePool::acquire(std::stop_token st) {
    std::size_t slot = 0;
    {
        std::unique_lock lock(mutex_);
        if (!available_.wait(lock, st, [this] { return !free_.empty(); })) {
            return fail(PipelineErrorCode::Cancelled, "cancelled while waiting for a host frame");
        }
        slot = free_.back();
        free_.pop_back();
    }
    return FrameBufferPtr{std::make_unique<Buffer>(shared_from_this(), slot)};
}

std::size_t HostFramePool::inUse() const {
    const std::scoped_lock lock(mutex_);
    return slots_.size() - free_.size();
}

// std::mutex::lock ne leve qu en cas d erreur systeme irrecuperable ; appele depuis un destructeur,
// std::terminate est alors le comportement voulu.
// NOLINTNEXTLINE(bugprone-exception-escape)
void HostFramePool::release(std::size_t slot) noexcept {
    {
        const std::scoped_lock lock(mutex_);
        free_.push_back(slot);
    }
    available_.notify_one();
}

} // namespace enh::pipeline
