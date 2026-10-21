#pragma once

// Pool borne d'images en memoire CPU (decodage logiciel sans GPU, faux de la CI). Toute la memoire
// est allouee a la creation ; acquire() bloque quand tout est pris (back-pressure) et s'interrompt
// par std::stop_token. Chaque image tient le pool en vie (shared_ptr) : aucune image ne pointe
// vers un pool detruit.

#include "enh/pipeline/core/frame_layout.hpp"

#include <condition_variable>
#include <cstddef>
#include <memory>
#include <mutex>
#include <stop_token>
#include <vector>

namespace enh::pipeline {

class HostFramePool : public std::enable_shared_from_this<HostFramePool> {
public:
    static constexpr std::size_t kPitchAlignment = 64;

    [[nodiscard]] static Result<std::shared_ptr<HostFramePool>> create(const FrameDesc& desc,
                                                                       std::size_t capacity);

    /// Image libre (pixels non initialises) ; Cancelled si st est arrete pendant l'attente.
    [[nodiscard]] Result<FrameBufferPtr> acquire(std::stop_token st);

    [[nodiscard]] const FrameDesc& desc() const noexcept { return desc_; }
    [[nodiscard]] const FrameLayout& layout() const noexcept { return layout_; }
    [[nodiscard]] std::size_t capacity() const noexcept { return slots_.size(); }
    [[nodiscard]] std::size_t inUse() const;

    HostFramePool(const HostFramePool&) = delete;
    HostFramePool& operator=(const HostFramePool&) = delete;
    HostFramePool(HostFramePool&&) = delete;
    HostFramePool& operator=(HostFramePool&&) = delete;
    ~HostFramePool() = default;

private:
    struct Slot {
        std::vector<std::byte> storage;
        std::byte* base = nullptr; // aligne sur kPitchAlignment dans storage
    };
    class Buffer;
    struct PrivateTag {};

public:
    HostFramePool(PrivateTag, const FrameDesc& desc, const FrameLayout& layout);

private:
    void release(std::size_t slot) noexcept;

    FrameDesc desc_;
    FrameLayout layout_;
    std::vector<Slot> slots_;
    mutable std::mutex mutex_;
    std::condition_variable_any available_;
    std::vector<std::size_t> free_;
};

} // namespace enh::pipeline
