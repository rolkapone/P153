// ENH-047 : VramMonitor avec une sonde factice (pic, repos, sonde muette, arret rapide et propre).
#include "enh/bench/vram_monitor.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <thread>
#include <utility>
#include <vector>

namespace enh::bench {
namespace {

using namespace std::chrono_literals;

/// Sonde dont la valeur est pilotee par le test.
class FakeProbe final : public IVramProbe {
public:
    void attachThread() noexcept override { attached.fetch_add(1); }
    std::optional<VramSample> sample() noexcept override {
        calls.fetch_add(1);
        if (mute.load()) {
            return std::nullopt;
        }
        return VramSample{.process_mib = process.load(), .device_used_mib = device.load()};
    }
    std::atomic<double> process{100.0};
    std::atomic<double> device{2000.0};
    std::atomic<bool> mute{false};
    std::atomic<int> calls{0};
    std::atomic<int> attached{0};
};

/// Sonde a sequence fixee : le releve n rend la valeur n de la liste (puis la derniere). Le
/// resultat ne depend pas de la vitesse de la machine : chaque releve compte, quel que soit le
/// moment ou il arrive.
class SequenceProbe final : public IVramProbe {
public:
    explicit SequenceProbe(std::vector<VramSample> seq) : seq_(std::move(seq)) {}
    std::optional<VramSample> sample() noexcept override {
        const auto n = static_cast<std::size_t>(calls.fetch_add(1));
        return seq_[std::min(n, seq_.size() - 1)];
    }
    std::atomic<int> calls{0};

private:
    std::vector<VramSample> seq_;
};

TEST(BenchVramMonitor, GardeLePicPasseger) {
    // Repos, pic d'un seul releve (n = 4), retour pres du repos.
    SequenceProbe probe({{100.0, 2000.0},
                         {100.0, 2000.0},
                         {110.0, 2010.0},
                         {150.0, 2050.0},
                         {400.0, 2300.0},
                         {130.0, 2030.0},
                         {120.0, 2020.0}});
    VramMonitor mon(probe, 1ms);
    const auto deadline = std::chrono::steady_clock::now() + 10s; // garde-fou, jamais atteint
    while (probe.calls.load() < 8 && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::yield();
    }
    const VramPeaks p = mon.stop();
    ASSERT_GE(probe.calls.load(), 8) << "le thread d'echantillonnage n'a pas tourne";
    EXPECT_DOUBLE_EQ(p.process_peak_mib, 400.0);
    EXPECT_DOUBLE_EQ(p.device_used_peak_mib, 2300.0);
    EXPECT_GE(p.samples, 8U);
}

TEST(BenchVramMonitor, AttacheLeThreadUneFois) {
    FakeProbe probe;
    VramMonitor mon(probe, 1ms);
    const auto deadline = std::chrono::steady_clock::now() + 10s;
    while (probe.calls.load() < 3 && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::yield();
    }
    (void)mon.stop();
    EXPECT_EQ(probe.attached.load(), 1);
}

TEST(BenchVramMonitor, ReleveSynchroneAuDebutEtALaFin) {
    FakeProbe probe;
    probe.process = 50.0;
    VramMonitor mon(probe, 1000ms); // periode longue : seuls les releves synchrones comptent
    probe.process = 70.0;           // valeur vue au moins par le releve final
    const VramPeaks p = mon.stop();
    EXPECT_DOUBLE_EQ(p.process_peak_mib, 70.0);
    EXPECT_GE(p.samples, 2U);
}

TEST(BenchVramMonitor, ArretRapide_MemeAvecUnePeriodeLongue) {
    FakeProbe probe;
    const auto t0 = std::chrono::steady_clock::now();
    {
        VramMonitor mon(probe, 1000ms);
        std::this_thread::sleep_for(10ms);
        (void)mon.stop();
    }
    EXPECT_LT(std::chrono::steady_clock::now() - t0, 500ms) << "stop() doit interrompre l'attente";
}

TEST(BenchVramMonitor, SondeMuette_AucunReleve) {
    FakeProbe probe;
    probe.mute = true;
    VramMonitor mon(probe, 1ms);
    std::this_thread::sleep_for(10ms);
    const VramPeaks p = mon.stop();
    EXPECT_EQ(p.samples, 0U);
    EXPECT_DOUBLE_EQ(p.process_peak_mib, 0.0);
    EXPECT_GT(probe.calls.load(), 0);
}

TEST(BenchVramMonitor, StopIdempotent_DestructeurSansStop) {
    FakeProbe probe;
    {
        VramMonitor mon(probe, 1ms);
        const VramPeaks a = mon.stop();
        const int calls = probe.calls.load();
        const VramPeaks b = mon.stop();
        EXPECT_EQ(a.samples, b.samples);
        EXPECT_EQ(probe.calls.load(), calls) << "pas de releve apres le premier stop()";
    }
    {
        VramMonitor mon(probe, 1ms);
    } // destructeur : jthread arrete et joint, pas de blocage
    SUCCEED();
}

} // namespace
} // namespace enh::bench
