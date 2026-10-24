#pragma once

// Environnement GPU du banc (exe seulement, pas de test CI) : sonde VRAM DXGI + CUDA, nom de la
// carte, version du pilote (NVML charge depuis System32), version de l'API pilote CUDA.

#include "enh/bench/vram_monitor.hpp"
#include "enh/pipeline/error.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

namespace enh::bench::gpu {

/// Usage VRAM du processus (DXGI QueryVideoMemoryInfo, segment local) sur l'adaptateur NVIDIA ;
/// avant l'initialisation CUDA, l'adaptateur est le premier de VendorId 0x10DE, ensuite celui du
/// LUID du peripherique CUDA 0 (rebindToCudaDevice).
class DxgiCudaProbe final : public IVramProbe {
public:
    /// nullptr si aucun adaptateur NVIDIA n'est visible par DXGI.
    [[nodiscard]] static std::unique_ptr<DxgiCudaProbe> createBeforeCuda();

    DxgiCudaProbe(const DxgiCudaProbe&) = delete;
    DxgiCudaProbe& operator=(const DxgiCudaProbe&) = delete;
    DxgiCudaProbe(DxgiCudaProbe&&) = delete;
    DxgiCudaProbe& operator=(DxgiCudaProbe&&) = delete;
    ~DxgiCudaProbe() override;

    /// Apres initPrimaryContext : choisit l'adaptateur du LUID CUDA et active cudaMemGetInfo.
    [[nodiscard]] pipeline::Result<void> rebindToCudaDevice();

    void attachThread() noexcept override;
    [[nodiscard]] std::optional<VramSample> sample() noexcept override;

    /// Usage du processus seul (0 si indisponible).
    [[nodiscard]] double processMib() const noexcept;

    struct Impl;

private:
    struct PrivateTag {};

public:
    DxgiCudaProbe(PrivateTag /*tag*/, std::unique_ptr<Impl> impl) noexcept;

private:
    std::unique_ptr<Impl> impl_;
};

struct GpuInfo {
    std::string name;
    std::string driver_version; // "617.42" (NVML), vide si NVML absent
    int cuda_driver_api = 0;    // cudaDriverGetVersion, ex. 13040
};

/// A appeler apres preloadNvidiaDriverDlls + initPrimaryContext.
[[nodiscard]] GpuInfo queryGpuInfo();

} // namespace enh::bench::gpu
