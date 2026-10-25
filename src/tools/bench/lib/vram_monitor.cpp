#include "enh/bench/vram_monitor.hpp"

namespace enh::bench {

namespace {

void raiseTo(std::atomic<double>& peak, double v) noexcept {
    double prev = peak.load(std::memory_order_relaxed);
    while (v > prev && !peak.compare_exchange_weak(prev, v, std::memory_order_relaxed)) {
    }
}

} // namespace

VramMonitor::VramMonitor(IVramProbe& probe, std::chrono::milliseconds period) : probe_(probe) {
    if (const auto s = probe_.sample()) {
        record(*s);
    }
    thread_ = std::jthread([this, period](const std::stop_token& st) {
        probe_.attachThread();
        while (!st.stop_requested()) {
            if (const auto s = probe_.sample()) {
                record(*s);
            }
            // Attente interrompue des que stop() est demande (pas de retard d'une periode).
            std::unique_lock lock(wait_mutex_);
            wait_cv_.wait_for(lock, st, period, [] { return false; });
        }
    });
}

void VramMonitor::record(const VramSample& s) noexcept {
    raiseTo(process_peak_, s.process_mib);
    raiseTo(device_peak_, s.device_used_mib);
    samples_.fetch_add(1, std::memory_order_relaxed);
}

VramPeaks VramMonitor::stop() {
    if (thread_.joinable()) {
        thread_.request_stop();
        thread_.join();
        if (const auto s = probe_.sample()) {
            record(*s);
        }
    }
    return {.process_peak_mib = process_peak_.load(),
            .device_used_peak_mib = device_peak_.load(),
            .samples = samples_.load()};
}

} // namespace enh::bench
