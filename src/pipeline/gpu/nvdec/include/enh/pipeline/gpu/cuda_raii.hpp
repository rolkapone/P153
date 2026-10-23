#pragma once

// RAII CUDA (runtime statique, ADR-048 : nvcuda.dll chargee a la demande, l'application demarre
// sans pilote NVIDIA). Le contexte est le contexte PRIMAIRE du peripherique (celui de cudart),
// partage avec FFmpeg/NVDEC, NVENC et TensorRT.

#include "enh/pipeline/error.hpp"

#include <cuda_runtime_api.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <source_location>
#include <string_view>
#include <type_traits>

namespace enh::pipeline::gpu {

/// cudaError_t -> PipelineError (memoire -> OutOfVram, pas de carte / pilote -> ComponentMissing,
/// faute GPU -> DeviceLost, sinon Internal).
[[nodiscard]] PipelineError
cudaFailure(cudaError_t err, std::string_view what,
            std::source_location where = std::source_location::current());

[[nodiscard]] inline Result<void>
checkCuda(cudaError_t err, std::string_view what,
          std::source_location where = std::source_location::current()) {
    if (err == cudaSuccess) {
        return {};
    }
    return enh::Unexpected<PipelineError>(cudaFailure(err, what, where));
}

struct StreamDeleter {
    void operator()(cudaStream_t s) const noexcept { cudaStreamDestroy(s); }
};
/// Flux CUDA non bloquant (cudaStreamCreateWithFlags -> cudaStreamDestroy).
using CudaStream = std::unique_ptr<std::remove_pointer_t<cudaStream_t>, StreamDeleter>;
[[nodiscard]] Result<CudaStream> makeStream();

struct EventDeleter {
    void operator()(cudaEvent_t e) const noexcept { cudaEventDestroy(e); }
};
/// Evenement sans chronometrage (cudaEventCreateWithFlags -> cudaEventDestroy).
using CudaEvent = std::unique_ptr<std::remove_pointer_t<cudaEvent_t>, EventDeleter>;
[[nodiscard]] Result<CudaEvent> makeEvent();

/// Memoire GPU a pas (cudaMallocPitch -> cudaFree).
class DeviceMemory {
public:
    [[nodiscard]] static Result<DeviceMemory> allocPitch(std::size_t width_bytes, std::size_t rows);
    DeviceMemory() = default;
    DeviceMemory(DeviceMemory&& o) noexcept;
    DeviceMemory& operator=(DeviceMemory&& o) noexcept;
    DeviceMemory(const DeviceMemory&) = delete;
    DeviceMemory& operator=(const DeviceMemory&) = delete;
    ~DeviceMemory();
    [[nodiscard]] std::uint64_t address() const noexcept {
        return reinterpret_cast<std::uint64_t>(ptr_);
    }
    [[nodiscard]] std::size_t pitch() const noexcept { return pitch_; }

private:
    void* ptr_ = nullptr;
    std::size_t pitch_ = 0;
};

/// Memoire hote epinglee (cudaHostAlloc -> cudaFreeHost).
class PinnedMemory {
public:
    [[nodiscard]] static Result<PinnedMemory> alloc(std::size_t bytes);
    PinnedMemory() = default;
    PinnedMemory(PinnedMemory&& o) noexcept;
    PinnedMemory& operator=(PinnedMemory&& o) noexcept;
    PinnedMemory(const PinnedMemory&) = delete;
    PinnedMemory& operator=(const PinnedMemory&) = delete;
    ~PinnedMemory();
    [[nodiscard]] std::byte* data() const noexcept { return static_cast<std::byte*>(ptr_); }

private:
    void* ptr_ = nullptr;
};

struct VramInfo {
    std::uint32_t free_mib = 0;
    std::uint32_t total_mib = 0;
};
/// VRAM du peripherique courant (cudaMemGetInfo ; tous processus confondus).
[[nodiscard]] Result<VramInfo> queryVram();

/// Precharge nvcuda, nvcuvid, nvEncodeAPI64, nvofapi64 et nvml depuis System32 seulement (SEC-08,
/// ENH-040) avant tout appel CUDA ou FFmpeg : les chargements ulterieurs par nom retrouvent ces
/// modules. ComponentMissing{Driver} si nvcuda.dll est absente. Idempotent, thread-safe, modules
/// gardes jusqu'a la fin du processus.
[[nodiscard]] Result<void> preloadNvidiaDriverDlls();

/// Initialise le contexte primaire du peripherique `device` sur le thread courant et le rend
/// (CUcontext, sous forme opaque) ; ComponentMissing si pas de carte ou de pilote.
[[nodiscard]] Result<void*> initPrimaryContext(int device);

} // namespace enh::pipeline::gpu
