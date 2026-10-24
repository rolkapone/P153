#pragma once

// Mesure de la VRAM pendant un etage (docs/gpu/ENH-047.md § 4) : un thread (std::jthread, arret
// par stop_token) interroge une sonde toutes les T ms et garde les maxima. La sonde est une
// interface : DXGI + cudaMemGetInfo dans le banc, sonde factice dans les tests CI.

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <optional>
#include <thread>

namespace enh::bench {

/// Un releve, en Mio.
struct VramSample {
    double process_mib = 0.0;     // usage du processus (DXGI QueryVideoMemoryInfo, segment local)
    double device_used_mib = 0.0; // toute la carte, tous processus (cudaMemGetInfo : total - libre)
};

class IVramProbe {
public:
    IVramProbe() = default;
    IVramProbe(const IVramProbe&) = delete;
    IVramProbe& operator=(const IVramProbe&) = delete;
    IVramProbe(IVramProbe&&) = delete;
    IVramProbe& operator=(IVramProbe&&) = delete;
    virtual ~IVramProbe() = default;

    /// Appele une fois dans le thread d'echantillonnage avant le premier releve (contexte CUDA).
    virtual void attachThread() noexcept {}
    /// Releve courant ; nullopt si la source ne repond pas (releve ignore). Thread-safe.
    [[nodiscard]] virtual std::optional<VramSample> sample() noexcept = 0;
};

struct VramPeaks {
    double process_peak_mib = 0.0;
    double device_used_peak_mib = 0.0;
    std::uint64_t samples = 0; // releves valides (dont le premier et le dernier, synchrones)
};

/// RAII : le constructeur fait un releve synchrone puis demarre le thread ; stop() (ou le
/// destructeur) l'arrete, fait un dernier releve synchrone et rend les maxima.
class VramMonitor {
public:
    VramMonitor(IVramProbe& probe, std::chrono::milliseconds period);
    VramMonitor(const VramMonitor&) = delete;
    VramMonitor& operator=(const VramMonitor&) = delete;
    VramMonitor(VramMonitor&&) = delete;
    VramMonitor& operator=(VramMonitor&&) = delete;
    ~VramMonitor() = default; // std::jthread : request_stop + join

    /// Arrete l'echantillonnage (idempotent) et rend les maxima.
    VramPeaks stop();

private:
    void record(const VramSample& s) noexcept;

    IVramProbe& probe_;
    std::atomic<double> process_peak_{0.0};
    std::atomic<double> device_peak_{0.0};
    std::atomic<std::uint64_t> samples_{0};
    std::mutex wait_mutex_; // attente interruptible entre deux releves
    std::condition_variable_any wait_cv_;
    std::jthread thread_; // dernier membre : demarre apres l'initialisation des atomiques
};

} // namespace enh::bench
