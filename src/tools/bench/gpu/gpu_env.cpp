#include "gpu_env.hpp"

#include <cuda_runtime_api.h>

#include <array>
#include <atomic>
#include <cstring>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
// clang-format off
#include <windows.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
// clang-format on

namespace enh::bench::gpu {

namespace {

using Microsoft::WRL::ComPtr;
constexpr UINT kNvidiaVendorId = 0x10DE;
constexpr double kMiB = 1024.0 * 1024.0;

} // namespace

struct DxgiCudaProbe::Impl {
    ComPtr<IDXGIAdapter3> adapter;               // ComPtr : Release() automatique (RAII COM)
    std::atomic<bool> cuda_ready{false};         // cudaMemGetInfo autorise (contexte cree)
    std::atomic<std::uint64_t> dxgi_failures{0}; // lectures QueryVideoMemoryInfo en echec
};

DxgiCudaProbe::DxgiCudaProbe(PrivateTag /*tag*/, std::unique_ptr<Impl> impl) noexcept
    : impl_(std::move(impl)) {}
DxgiCudaProbe::~DxgiCudaProbe() = default;

std::unique_ptr<DxgiCudaProbe> DxgiCudaProbe::createBeforeCuda() {
    ComPtr<IDXGIFactory1> factory;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) {
        return nullptr;
    }
    ComPtr<IDXGIAdapter1> adapter;
    for (UINT i = 0; factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
        DXGI_ADAPTER_DESC1 desc{};
        if (SUCCEEDED(adapter->GetDesc1(&desc)) && desc.VendorId == kNvidiaVendorId) {
            auto impl = std::make_unique<Impl>();
            if (SUCCEEDED(adapter.As(&impl->adapter))) {
                return std::make_unique<DxgiCudaProbe>(PrivateTag{}, std::move(impl));
            }
        }
        adapter.Reset();
    }
    return nullptr;
}

pipeline::Result<void> DxgiCudaProbe::rebindToCudaDevice() {
    cudaDeviceProp prop{};
    if (const cudaError_t e = cudaGetDeviceProperties(&prop, 0); e != cudaSuccess) {
        return pipeline::fail(pipeline::PipelineErrorCode::ComponentMissing,
                              "cudaGetDeviceProperties failed", e);
    }
    ComPtr<IDXGIFactory4> factory;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) {
        return pipeline::fail(pipeline::PipelineErrorCode::Internal, "CreateDXGIFactory1 failed");
    }
    LUID luid{};
    static_assert(sizeof luid <= sizeof prop.luid);
    std::memcpy(&luid, prop.luid, sizeof luid);
    ComPtr<IDXGIAdapter3> adapter;
    if (FAILED(factory->EnumAdapterByLuid(luid, IID_PPV_ARGS(&adapter)))) {
        return pipeline::fail(pipeline::PipelineErrorCode::Internal,
                              "no DXGI adapter for the CUDA device LUID");
    }
    impl_->adapter = adapter;
    impl_->cuda_ready.store(true);
    return {};
}

void DxgiCudaProbe::attachThread() noexcept {
    if (impl_->cuda_ready.load()) {
        (void)cudaSetDevice(0); // contexte primaire du peripherique 0 dans ce thread
    }
}

double DxgiCudaProbe::processMib() const noexcept {
    DXGI_QUERY_VIDEO_MEMORY_INFO info{};
    if (impl_->adapter == nullptr ||
        FAILED(impl_->adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info))) {
        impl_->dxgi_failures.fetch_add(1);
        return 0.0;
    }
    return static_cast<double>(info.CurrentUsage) / kMiB;
}

std::uint64_t DxgiCudaProbe::dxgiFailures() const noexcept {
    return impl_->dxgi_failures.load();
}

std::optional<VramSample> DxgiCudaProbe::sample() noexcept {
    VramSample s;
    s.process_mib = processMib();
    if (impl_->cuda_ready.load()) {
        std::size_t free_b = 0;
        std::size_t total_b = 0;
        if (cudaMemGetInfo(&free_b, &total_b) == cudaSuccess) {
            s.device_used_mib = static_cast<double>(total_b - free_b) / kMiB;
        }
    }
    return s;
}

GpuInfo queryGpuInfo() {
    GpuInfo info;
    cudaDeviceProp prop{};
    if (cudaGetDeviceProperties(&prop, 0) == cudaSuccess) {
        info.name = prop.name;
    }
    (void)cudaDriverGetVersion(&info.cuda_driver_api);
    // NVML : deja chargee depuis System32 par preloadNvidiaDriverDlls (SEC-08) ; on ne charge rien
    // par nom ici, on se contente du module present.
    HMODULE nvml = GetModuleHandleW(L"nvml.dll");
    if (nvml == nullptr) {
        return info;
    }
    using InitFn = int (*)();
    using VersionFn = int (*)(char*, unsigned int);
    using ShutdownFn = int (*)();
    const auto init = reinterpret_cast<InitFn>(GetProcAddress(nvml, "nvmlInit_v2"));
    const auto version =
        reinterpret_cast<VersionFn>(GetProcAddress(nvml, "nvmlSystemGetDriverVersion"));
    const auto shutdown = reinterpret_cast<ShutdownFn>(GetProcAddress(nvml, "nvmlShutdown"));
    if (init == nullptr || version == nullptr || shutdown == nullptr || init() != 0) {
        return info;
    }
    std::array<char, 96> buf{};
    if (version(buf.data(), static_cast<unsigned int>(buf.size())) == 0) {
        info.driver_version = buf.data();
    }
    (void)shutdown();
    return info;
}

} // namespace enh::bench::gpu
