#include "enh/pipeline/gpu/cuda_raii.hpp"

#include <cuda.h>

#include <array>
#include <mutex>
#include <string>
#include <utility>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace enh::pipeline::gpu {

namespace {
constexpr std::size_t kMiB = std::size_t{1024} * 1024;
// DLL du pilote chargees par nom par FFmpeg (ffnvcodec), NGX, NVENC, l'Optical Flow SDK et NVML :
// prechargees depuis System32 pour que ces chargements par nom retrouvent ces modules (SEC-08).
constexpr std::array<const wchar_t*, 4> kOptionalDriverDlls = {L"nvcuvid.dll", L"nvEncodeAPI64.dll",
                                                               L"nvofapi64.dll", L"nvml.dll"};
} // namespace

PipelineError cudaFailure(cudaError_t err, std::string_view what, std::source_location where) {
    PipelineErrorCode code = PipelineErrorCode::Internal;
    Component component = Component::None;
    switch (err) {
    case cudaErrorMemoryAllocation:
        code = PipelineErrorCode::OutOfVram;
        break;
    case cudaErrorNoDevice:
    case cudaErrorInvalidDevice:
        code = PipelineErrorCode::ComponentMissing;
        component = Component::NvidiaGpu;
        break;
    case cudaErrorInsufficientDriver:
        code = PipelineErrorCode::ComponentMissing;
        component = Component::Driver;
        break;
    case cudaErrorLaunchFailure:
    case cudaErrorIllegalAddress:
    case cudaErrorDevicesUnavailable:
    case cudaErrorHardwareStackError:
    case cudaErrorUnknown:
        code = PipelineErrorCode::DeviceLost;
        break;
    default:
        break;
    }
    std::string detail(what);
    detail += ": ";
    detail += cudaGetErrorName(err);
    detail += " (";
    detail += cudaGetErrorString(err);
    detail += ')';
    PipelineError e = makeError(code, std::move(detail), static_cast<std::int64_t>(err), where);
    e.component = component;
    if (code == PipelineErrorCode::OutOfVram) {
        std::size_t free_b = 0;
        std::size_t total_b = 0;
        if (cudaMemGetInfo(&free_b, &total_b) == cudaSuccess) {
            e.available_mib = static_cast<std::uint32_t>(free_b / kMiB);
        }
    }
    return e;
}

Result<CudaStream> makeStream() {
    cudaStream_t s = nullptr;
    if (auto r = checkCuda(cudaStreamCreateWithFlags(&s, cudaStreamNonBlocking),
                           "cudaStreamCreateWithFlags");
        !r) {
        return enh::Unexpected<PipelineError>(std::move(r.error()));
    }
    return CudaStream(s);
}

Result<CudaEvent> makeEvent() {
    cudaEvent_t e = nullptr;
    if (auto r = checkCuda(cudaEventCreateWithFlags(&e, cudaEventDisableTiming),
                           "cudaEventCreateWithFlags");
        !r) {
        return enh::Unexpected<PipelineError>(std::move(r.error()));
    }
    return CudaEvent(e);
}

Result<DeviceMemory> DeviceMemory::allocPitch(std::size_t width_bytes, std::size_t rows) {
    DeviceMemory m;
    if (auto r =
            checkCuda(cudaMallocPitch(&m.ptr_, &m.pitch_, width_bytes, rows), "cudaMallocPitch");
        !r) {
        PipelineError e = std::move(r.error());
        e.required_mib = static_cast<std::uint32_t>((width_bytes * rows + kMiB - 1) / kMiB);
        return enh::Unexpected<PipelineError>(std::move(e));
    }
    return m;
}

DeviceMemory::DeviceMemory(DeviceMemory&& o) noexcept
    : ptr_(std::exchange(o.ptr_, nullptr)), pitch_(std::exchange(o.pitch_, 0)) {}

DeviceMemory& DeviceMemory::operator=(DeviceMemory&& o) noexcept {
    if (this != &o) {
        if (ptr_ != nullptr) {
            cudaFree(ptr_);
        }
        ptr_ = std::exchange(o.ptr_, nullptr);
        pitch_ = std::exchange(o.pitch_, 0);
    }
    return *this;
}

DeviceMemory::~DeviceMemory() {
    if (ptr_ != nullptr) {
        cudaFree(ptr_);
    }
}

Result<PinnedMemory> PinnedMemory::alloc(std::size_t bytes) {
    PinnedMemory m;
    if (auto r = checkCuda(cudaHostAlloc(&m.ptr_, bytes, cudaHostAllocDefault), "cudaHostAlloc");
        !r) {
        return enh::Unexpected<PipelineError>(std::move(r.error()));
    }
    return m;
}

PinnedMemory::PinnedMemory(PinnedMemory&& o) noexcept : ptr_(std::exchange(o.ptr_, nullptr)) {}

PinnedMemory& PinnedMemory::operator=(PinnedMemory&& o) noexcept {
    if (this != &o) {
        if (ptr_ != nullptr) {
            cudaFreeHost(ptr_);
        }
        ptr_ = std::exchange(o.ptr_, nullptr);
    }
    return *this;
}

PinnedMemory::~PinnedMemory() {
    if (ptr_ != nullptr) {
        cudaFreeHost(ptr_);
    }
}

Result<VramInfo> queryVram() {
    std::size_t free_b = 0;
    std::size_t total_b = 0;
    if (auto r = checkCuda(cudaMemGetInfo(&free_b, &total_b), "cudaMemGetInfo"); !r) {
        return enh::Unexpected<PipelineError>(std::move(r.error()));
    }
    return VramInfo{.free_mib = static_cast<std::uint32_t>(free_b / kMiB),
                    .total_mib = static_cast<std::uint32_t>(total_b / kMiB)};
}

Result<void> preloadNvidiaDriverDlls() {
    static std::once_flag once;
    static bool cuda_ok = false;
    std::call_once(once, [] {
        // Handles gardes volontairement jusqu'a la fin du processus (modules du pilote).
        cuda_ok = LoadLibraryExW(L"nvcuda.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32) != nullptr;
        // Les autres sont facultatives ici (absentes = fonction indisponible, signalee plus tard).
        for (const wchar_t* name : kOptionalDriverDlls) {
            (void)LoadLibraryExW(name, nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        }
    });
    if (!cuda_ok) {
        PipelineError e = makeError(PipelineErrorCode::ComponentMissing,
                                    "nvcuda.dll not found in System32 (NVIDIA driver missing)");
        e.component = Component::Driver;
        return enh::Unexpected<PipelineError>(std::move(e));
    }
    return {};
}

Result<void*> initPrimaryContext(int device) {
    if (auto r = preloadNvidiaDriverDlls(); !r) {
        return enh::Unexpected<PipelineError>(std::move(r.error()));
    }
    if (auto r = checkCuda(cudaSetDevice(device), "cudaSetDevice"); !r) {
        return enh::Unexpected<PipelineError>(std::move(r.error()));
    }
    if (auto r = checkCuda(cudaFree(nullptr), "cudaFree(0) context init"); !r) {
        return enh::Unexpected<PipelineError>(std::move(r.error()));
    }
    // cuCtxGetCurrent par le point d'entree du pilote : pas de liaison a cuda.lib (ADR-048).
    void* fn = nullptr;
    cudaDriverEntryPointQueryResult status{};
    if (auto r = checkCuda(cudaGetDriverEntryPointByVersion("cuCtxGetCurrent", &fn, 12000,
                                                            cudaEnableDefault, &status),
                           "cudaGetDriverEntryPointByVersion(cuCtxGetCurrent)");
        !r) {
        return enh::Unexpected<PipelineError>(std::move(r.error()));
    }
    if (fn == nullptr || status != cudaDriverEntryPointSuccess) {
        PipelineError e =
            makeError(PipelineErrorCode::ComponentMissing, "cuCtxGetCurrent unavailable");
        e.component = Component::Driver;
        return enh::Unexpected<PipelineError>(std::move(e));
    }
    using GetCurrentFn = CUresult(CUDAAPI*)(CUcontext*);
    CUcontext ctx = nullptr;
    const CUresult cr = reinterpret_cast<GetCurrentFn>(fn)(&ctx);
    if (cr != CUDA_SUCCESS || ctx == nullptr) {
        return fail(PipelineErrorCode::Internal, "cuCtxGetCurrent failed", cr);
    }
    return static_cast<void*>(ctx);
}

} // namespace enh::pipeline::gpu
