// ENH-040 sur la RTX (local seulement, label gpu) : pont RTX Video sans cudart, super-resolution
// 1280x720 -> 3840x2160 d'une image synthetique (q4 != q0), debit q4 8 et 10 bits, VRAM de l'effet,
// 10 cycles creation / destruction sans derive, provenance et signature des DLL chargees,
// aucun journal ni fichier ecrit par NGX (preuves cote application de SEC-01 ; reseau : ENH-041).
#include "enh/pipeline/core/log.hpp"
#include "enh/pipeline/gpu/cuda_raii.hpp"
#include "enh/pipeline/gpu/cuda_rgb_image.hpp"
#include "enh/pipeline/rtxvideo/bridge_library.hpp"
#include "enh/pipeline/rtxvideo/module_trust.hpp"
#include "enh/pipeline/rtxvideo/rtxvideo_upscaler.hpp"

#include "pe/pe_image.hpp"

#include <cuda_runtime_api.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <mutex>
#include <numbers>
#include <string>
#include <thread>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
// clang-format off
#include <windows.h>
#include <dxgi1_4.h>
#include <psapi.h>
// clang-format on

namespace enh::pipeline {
namespace {

namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;
using gpu::CudaRgbImage;
using rtxvideo::RtxVideoUpscaler;

constexpr Size kIn{.width = 1280, .height = 720};
constexpr Size kOut{.width = 3840, .height = 2160};
constexpr std::uint32_t kMiB = 1024 * 1024;

// ---------------------------------------------------------------------------------------------
// Outils de mesure
// ---------------------------------------------------------------------------------------------

/// VRAM du PROCESSUS (DXGI QueryVideoMemoryInfo, segment local), comme le banc du spike.
class ProcessVram {
public:
    ProcessVram() {
        cudaDeviceProp prop{};
        if (cudaGetDeviceProperties(&prop, 0) != cudaSuccess) {
            return;
        }
        IDXGIFactory4* factory = nullptr;
        if (FAILED(
                CreateDXGIFactory1(__uuidof(IDXGIFactory4), reinterpret_cast<void**>(&factory)))) {
            return;
        }
        LUID luid{};
        std::memcpy(&luid, prop.luid, sizeof luid);
        IDXGIAdapter3* adapter = nullptr;
        (void)factory->EnumAdapterByLuid(luid, __uuidof(IDXGIAdapter3),
                                         reinterpret_cast<void**>(&adapter));
        factory->Release();
        adapter_ = adapter;
    }
    ProcessVram(const ProcessVram&) = delete;
    ProcessVram& operator=(const ProcessVram&) = delete;
    ProcessVram(ProcessVram&&) = delete;
    ProcessVram& operator=(ProcessVram&&) = delete;
    ~ProcessVram() {
        if (adapter_ != nullptr) {
            adapter_->Release();
        }
    }
    [[nodiscard]] bool ok() const { return adapter_ != nullptr; }
    [[nodiscard]] double mib() const {
        DXGI_QUERY_VIDEO_MEMORY_INFO info{};
        if (adapter_ == nullptr ||
            FAILED(adapter_->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info))) {
            return 0.0;
        }
        return static_cast<double>(info.CurrentUsage) / kMiB;
    }

private:
    IDXGIAdapter3* adapter_ = nullptr;
};

/// Pic de VRAM du processus, echantillonne toutes les 10 ms (protocole QA §4.5).
class PeakSampler {
public:
    explicit PeakSampler(const ProcessVram& vram)
        : vram_(vram), peak_(vram.mib()), thread_([this](const std::stop_token& st) {
              while (!st.stop_requested()) {
                  const double v = vram_.mib();
                  double prev = peak_.load();
                  while (v > prev && !peak_.compare_exchange_weak(prev, v)) {
                  }
                  std::this_thread::sleep_for(std::chrono::milliseconds(10));
              }
          }) {}
    double stop() {
        thread_.request_stop();
        thread_.join();
        return std::max(peak_.load(), vram_.mib());
    }

private:
    const ProcessVram& vram_;
    std::atomic<double> peak_;
    std::jthread thread_;
};

DWORD handleCount() {
    DWORD n = 0;
    GetProcessHandleCount(GetCurrentProcess(), &n);
    return n;
}

fs::path exeDir() {
    auto d = rtxvideo::applicationDirectory();
    return d ? *d : fs::path{};
}

fs::path systemDir() {
    std::array<wchar_t, MAX_PATH> sys{};
    GetSystemDirectoryW(sys.data(), MAX_PATH);
    return {sys.data()};
}

std::vector<char> readAll(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), {}};
}

std::string lower(std::string s) {
    std::ranges::transform(s, s.begin(), [](char c) {
        return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    });
    return s;
}

/// Image synthetique (CPU) : degrades, mire de frequences, damier, disques, traits fins.
std::vector<std::uint32_t> syntheticImage(Size s, RgbFormat f) {
    std::vector<std::uint32_t> px(static_cast<std::size_t>(s.width) * s.height);
    for (std::uint32_t y = 0; y < s.height; ++y) {
        for (std::uint32_t x = 0; x < s.width; ++x) {
            const double u = static_cast<double>(x) / s.width;
            const double v = static_cast<double>(y) / s.height;
            double r = u;
            double g = v;
            double b = 0.5 + 0.5 * std::sin(40.0 * u * u * std::numbers::pi);
            if (((x / 16) + (y / 16)) % 2 == 0 && x > s.width / 2) {
                r = 1.0 - r; // damier
            }
            const double dx = x - s.width * 0.25;
            const double dy = y - s.height * 0.5;
            if (dx * dx + dy * dy < 150.0 * 150.0) {
                g = 1.0 - g; // disque
            }
            if (y % 37 == 0 || x % 53 == 0) {
                r = g = b = 1.0; // traits fins
            }
            if (f == RgbFormat::Argb8) {
                const auto c = [](double t) {
                    return static_cast<std::uint32_t>(std::lround(t * 255.0));
                };
                px[std::size_t{y} * s.width + x] =
                    c(r) | (c(g) << 8U) | (c(b) << 16U) | (255U << 24U);
            } else {
                const auto c = [](double t) {
                    return static_cast<std::uint32_t>(std::lround(t * 1023.0));
                };
                px[std::size_t{y} * s.width + x] =
                    c(r) | (c(g) << 10U) | (c(b) << 20U) | (3U << 30U);
            }
        }
    }
    return px;
}

double differingBytesRatio(const std::vector<std::uint32_t>& a,
                           const std::vector<std::uint32_t>& b) {
    std::size_t diff = 0;
    for (std::size_t i = 0; i < a.size(); ++i) {
        const std::uint32_t x = a[i] ^ b[i];
        for (unsigned k = 0; k < 4; ++k) {
            diff += ((x >> (8U * k)) & 0xFFU) != 0 ? 1 : 0;
        }
    }
    return static_cast<double>(diff) / (static_cast<double>(a.size()) * 4.0);
}

const char* bitsName(RgbFormat f) {
    return f == RgbFormat::Argb8 ? "8 bits" : "10 bits";
}

// ---------------------------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------------------------

class ENH040 : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        auto ctx = gpu::initPrimaryContext(0);
        ASSERT_TRUE(ctx) << ctx.error().detail;
        context_ = *ctx;
        auto s = gpu::makeStream();
        ASSERT_TRUE(s) << s.error().detail;
        stream_ = std::move(*s);
    }
    static void TearDownTestSuite() { stream_.reset(); }

    static fs::path ngxDataDir() { return fs::temp_directory_path() / "enh_gpu_tests" / "ngx"; }

    static Result<std::unique_ptr<RtxVideoUpscaler>> makeUpscaler() {
        rtxvideo::RtxVideoUpscalerConfig cfg;
        cfg.ngx_data_dir = ngxDataDir(); // jamais le dossier de l'utilisateur pendant les tests
        cfg.cu_context = context_;
        cfg.cu_stream = stream_.get();
        return RtxVideoUpscaler::create(cfg);
    }

    static cudaStream_t stream() { return stream_.get(); }

    static UpscaleRequest request(const CudaRgbImage& in, const CudaRgbImage& out,
                                  UpscaleQuality q) {
        UpscaleRequest r;
        r.input_texture = in.texture();
        r.input_size_px = in.size();
        r.output_surface = out.surface();
        r.output_size_px = out.size();
        r.quality = q;
        return r;
    }

    static void* context_;
    static gpu::CudaStream stream_;
};

void* ENH040::context_ = nullptr;
gpu::CudaStream ENH040::stream_;

class ENH040Bits : public ENH040, public ::testing::WithParamInterface<RgbFormat> {};

// ---------------------------------------------------------------------------------------------
// Pont : imports, exports, DLL de la fonction
// ---------------------------------------------------------------------------------------------

TEST_F(ENH040, Pont_AucunCudartImporte_ExportsAttendus) {
    const auto info = tests::readPe(exeDir() / rtxvideo::kBridgeDllName);
    if (!info.has_value()) {
        FAIL() << "enh_rtxvideo_bridge.dll absente ou illisible a cote du test";
    }
    const tests::PeInfo& pe = *info;
    std::cout << "[ENH-040] imports du pont :";
    for (const auto& list : {pe.imports, pe.delay_imports}) {
        for (const std::string& dll : list) {
            std::cout << ' ' << dll;
            EXPECT_EQ(lower(dll).find("cudart"), std::string::npos) << dll;
        }
    }
    std::cout << '\n';
    // Notre API : les 7 fonctions rvb_*. Les substituts cuda* ne sont PAS exportes.
    // nvsdk_ngx_s.lib porte ses propres directives d'export (NVSDK_NGX_*) : l'editeur de liens les
    // ajoute, sans moyen de les retirer ; jamais resolues par Enhance (GetProcAddress rvb_* seul).
    std::vector<std::string> ours;
    std::size_t ngx = 0;
    for (const std::string& e : pe.exports) {
        if (e.starts_with("rvb_")) {
            ours.push_back(e);
        } else if (e.starts_with("NVSDK_NGX_")) {
            ++ngx;
        } else {
            ADD_FAILURE() << "export inattendu : " << e;
        }
    }
    std::ranges::sort(ours);
    const std::vector<std::string> expected = {
        "rvb_abi_version", "rvb_effect_destroy", "rvb_runtime_create", "rvb_runtime_destroy",
        "rvb_vsr_create",  "rvb_vsr_evaluate",   "rvb_vsr_query"};
    EXPECT_EQ(ours, expected);
    std::cout << "[ENH-040] exports : " << ours.size() << " rvb_*, " << ngx
              << " NVSDK_NGX_* (directives de nvsdk_ngx_s.lib)\n";
}

TEST_F(ENH040, DllFonction_CopieRel_SigneeNvidia) {
    const fs::path deployed = exeDir() / rtxvideo::kVsrFeatureDllName;
    const fs::path sdk = fs::path(ENH_RTXVIDEO_SDK_DIR) / L"bin" / L"Windows" / L"x64";
    ASSERT_TRUE(fs::exists(deployed));
    const auto bytes = readAll(deployed);
    EXPECT_EQ(bytes, readAll(sdk / L"rel" / L"nvngx_vsr.dll")) << "copie differente de bin/rel";
    EXPECT_NE(bytes, readAll(sdk / L"dev" / L"nvngx_vsr.dll")) << "DLL dev (filigrane) deployee";
    auto signer = rtxvideo::verifyAuthenticodeOffline(deployed);
    ASSERT_TRUE(signer) << signer.error().detail;
    EXPECT_EQ(*signer, rtxvideo::kNvidiaSigner);
    std::wcout << L"[ENH-040] nvngx_vsr.dll signee par : " << *signer << L'\n';
}

// ---------------------------------------------------------------------------------------------
// Super-resolution : sortie, modules charges, journal
// ---------------------------------------------------------------------------------------------

TEST_P(ENH040Bits, Synthetique720pVers4K_Q4DiffereDeQ0) {
    const RgbFormat fmt = GetParam();
    auto in = CudaRgbImage::create(kIn, fmt);
    auto out = CudaRgbImage::create(kOut, fmt);
    ASSERT_TRUE(in && out);
    const auto src = syntheticImage(kIn, fmt);
    ASSERT_TRUE(in->upload(src, stream()));

    const auto t0 = Clock::now();
    auto up = makeUpscaler(); // aucun 0xC0000409 : le pont isole NGX du cudart 13
    ASSERT_TRUE(up) << up.error().detail;
    const double create_s = std::chrono::duration<double>(Clock::now() - t0).count();

    std::vector<std::uint32_t> q0(out->pixelCount());
    std::vector<std::uint32_t> q4(out->pixelCount());
    ASSERT_TRUE((*up)->upscale(request(*in, *out, UpscaleQuality::Bicubic)));
    ASSERT_TRUE(out->download(q0, stream()));
    ASSERT_EQ(cudaStreamSynchronize(stream()), cudaSuccess);
    ASSERT_TRUE((*up)->upscale(request(*in, *out, UpscaleQuality::Ultra)));
    ASSERT_TRUE(out->download(q4, stream()));
    ASSERT_EQ(cudaStreamSynchronize(stream()), cudaSuccess);

    const double ratio = differingBytesRatio(q0, q4);
    const auto non_zero =
        std::ranges::count_if(q4, [](std::uint32_t p) { return (p & 0x3FFFFFFFU) != 0; });
    std::cout << "[ENH-040] " << bitsName(fmt) << " : octets q4 != q0 = " << ratio * 100.0
              << " %, creation de l'effet " << create_s << " s\n";
    RecordProperty("q4_vs_q0_diff_pct", std::to_string(ratio * 100.0));
    EXPECT_GT(ratio, 0.10); // note GPU § 6 (spike : 59 % sur image reelle ; QA : >= 30 % reelle)
    EXPECT_GT(non_zero, static_cast<std::ptrdiff_t>(q4.size() / 2)) << "sortie quasi vide";
}

TEST_F(ENH040, ModulesEtDllDeFonction_Provenance) {
    auto up = makeUpscaler();
    ASSERT_TRUE(up) << up.error().detail;
    // SEC-01 : la DLL de fonction executee est celle livree (ou sa copie identique dans le magasin
    // NGX) ; la mise a jour NGX (processus enfant) a ete bloquee ; aucun processus cree.
    const rtxvideo::VsrProvenance& prov = (*up)->provenance();
    std::cout << "[ENH-040] DLL de fonction VSR : " << rtxvideo::toString(prov.source) << ' '
              << prov.version << ", mise a jour bloquee : " << prov.updater_blocked
              << ", processus enfants crees : " << prov.child_processes_created << '\n';
    EXPECT_TRUE(prov.source == rtxvideo::SnippetSource::AppFolder ||
                prov.source == rtxvideo::SnippetSource::NgxStoreIdentical);
    EXPECT_TRUE(prov.updater_blocked);
    EXPECT_EQ(prov.child_processes_created, 0U);
    // Pont : dossier de l'application.
    const auto bridge = rtxvideo::loadedModulePaths(rtxvideo::kBridgeDllName);
    ASSERT_EQ(bridge.size(), 1U);
    EXPECT_TRUE(rtxvideo::sameFile(bridge.front(), exeDir() / rtxvideo::kBridgeDllName));
    // SEC-08 : DLL du pilote prechargees depuis System32.
    for (const wchar_t* name :
         {L"nvcuda.dll", L"nvcuvid.dll", L"nvEncodeAPI64.dll", L"nvofapi64.dll", L"nvml.dll"}) {
        const auto paths = rtxvideo::loadedModulePaths(name);
        ASSERT_FALSE(paths.empty()) << "non prechargee";
        for (const fs::path& p : paths) {
            EXPECT_TRUE(rtxvideo::sameFile(p.parent_path(), systemDir())) << p.string();
        }
    }
    // Aucun runtime CUDA en DLL dans le processus (cause de B4). NGX charge ses propres modules
    // sans le chargeur Windows (invisibles ici) : d'ou la verification par le journal ci-dessus.
    std::vector<HMODULE> mods(1024);
    DWORD needed = 0;
    ASSERT_NE(EnumProcessModules(GetCurrentProcess(), mods.data(),
                                 static_cast<DWORD>(mods.size() * sizeof(HMODULE)), &needed),
              FALSE);
    mods.resize(needed / sizeof(HMODULE));
    for (HMODULE m : mods) {
        std::array<wchar_t, MAX_PATH> buf{};
        GetModuleFileNameW(m, buf.data(), MAX_PATH);
        const std::string name = lower(fs::path(buf.data()).filename().string());
        EXPECT_FALSE(name.starts_with("cudart")) << name;
    }
}

TEST_F(ENH040, MiseAJourNgx_ProcessusBloque_Trace) {
    // Journal NGX detaille capture en memoire (test seulement) : la tentative de lancement de
    // nvngx_update.exe doit echouer, aucun processus ne doit apparaitre dans le job.
    std::mutex mutex;
    std::vector<std::string> updater_lines;
    setLogSink([&](LogLevel, std::string_view code, std::string_view msg) {
        if (code == "NgxLog" && msg.find("Updater") != std::string_view::npos) {
            const std::scoped_lock lock(mutex);
            updater_lines.emplace_back(msg);
        }
    });
    const std::uint32_t before = rtxvideo::ChildProcessBlocker::childProcessesCreated();
    {
        rtxvideo::RtxVideoUpscalerConfig cfg;
        cfg.ngx_data_dir = ngxDataDir();
        cfg.cu_context = context_;
        cfg.cu_stream = stream_.get();
        cfg.log_level = rtxvideo::NgxLogLevel::Verbose;
        auto up = RtxVideoUpscaler::create(cfg);
        ASSERT_TRUE(up) << up.error().detail;
    }
    setLogSink({});
    EXPECT_EQ(rtxvideo::ChildProcessBlocker::childProcessesCreated(), before);
    for (const std::string& l : updater_lines) {
        std::cout << "[ENH-040] NGX : "
                  << l.substr(l.find("[Launch") == std::string::npos ? 0 : l.find("[Launch"))
                  << '\n';
        EXPECT_EQ(l.find("created successfully"), std::string::npos);
    }
}

/// Fichiers (nom, taille, date) sous un dossier : comparaison avant / apres.
std::map<std::wstring, std::pair<std::uintmax_t, fs::file_time_type>>
snapshot(const fs::path& dir) {
    std::map<std::wstring, std::pair<std::uintmax_t, fs::file_time_type>> out;
    std::error_code ec;
    if (!fs::exists(dir, ec)) {
        return out;
    }
    for (auto it = fs::recursive_directory_iterator(
             dir, fs::directory_options::skip_permission_denied, ec);
         !ec && it != fs::recursive_directory_iterator(); it.increment(ec)) {
        if (it->is_regular_file(ec)) {
            out[it->path().wstring()] = {it->file_size(ec), it->last_write_time(ec)};
        }
    }
    return out;
}

TEST_F(ENH040, NgxNEcritNiJournalNiMiseAJour) {
    std::error_code ec;
    fs::remove_all(ngxDataDir(), ec);
    std::array<wchar_t, MAX_PATH> program_data{};
    GetEnvironmentVariableW(L"ProgramData", program_data.data(), MAX_PATH);
    const fs::path ngx_store = fs::path(program_data.data()) / L"NVIDIA" / L"NGX";
    const auto before = snapshot(ngx_store);
    {
        auto in = CudaRgbImage::create(kIn, RgbFormat::Argb8);
        auto out = CudaRgbImage::create(kOut, RgbFormat::Argb8);
        ASSERT_TRUE(in && out);
        auto up = makeUpscaler();
        ASSERT_TRUE(up) << up.error().detail;
        ASSERT_TRUE((*up)->upscale(request(*in, *out, UpscaleQuality::Ultra)));
        ASSERT_EQ(cudaStreamSynchronize(stream()), cudaSuccess);
    }
    // Journal NGX coupe : aucun fichier dans le dossier de donnees.
    const auto written = snapshot(ngxDataDir());
    for (const auto& entry : written) {
        ADD_FAILURE() << "fichier ecrit par NGX : " << fs::path(entry.first).filename().string();
    }
    // Magasin de mises a jour NGX du systeme : inchange (aucune DLL telechargee ou remplacee).
    EXPECT_EQ(snapshot(ngx_store), before) << "%ProgramData%\\NVIDIA\\NGX modifie";
    std::cout << "[ENH-040] dossier NGX de l'appli : " << written.size()
              << " fichier(s) ; magasin systeme NGX : " << before.size()
              << " fichier(s), inchange\n";
}

// ---------------------------------------------------------------------------------------------
// Mesures : debit, VRAM, cycles
// ---------------------------------------------------------------------------------------------

TEST_P(ENH040Bits, Debit_Q4_720pVers4K) {
    const RgbFormat fmt = GetParam();
    constexpr int kWarmup = 30;
    constexpr int kFrames = 300;
    constexpr int kRepeats = 5;
    std::vector<CudaRgbImage> inputs;
    for (int i = 0; i < 2; ++i) {
        auto in = CudaRgbImage::create(kIn, fmt);
        ASSERT_TRUE(in);
        auto px = syntheticImage(kIn, fmt);
        std::ranges::rotate(px, px.begin() + static_cast<std::ptrdiff_t>(i) * 977); // 2 images
        ASSERT_TRUE(in->upload(px, stream()));
        ASSERT_EQ(cudaStreamSynchronize(stream()), cudaSuccess);
        inputs.push_back(std::move(*in));
    }
    auto out = CudaRgbImage::create(kOut, fmt);
    ASSERT_TRUE(out);
    auto up = makeUpscaler();
    ASSERT_TRUE(up) << up.error().detail;

    std::vector<double> fps;
    for (int rep = 0; rep < kRepeats; ++rep) {
        Clock::time_point start;
        for (int i = 0; i < kWarmup + kFrames; ++i) {
            if (i == kWarmup) {
                ASSERT_EQ(cudaStreamSynchronize(stream()), cudaSuccess);
                start = Clock::now();
            }
            const auto r = (*up)->upscale(
                request(inputs[static_cast<std::size_t>(i % 2)], *out, UpscaleQuality::Ultra));
            ASSERT_TRUE(r) << r.error().detail;
        }
        ASSERT_EQ(cudaStreamSynchronize(stream()), cudaSuccess);
        fps.push_back(kFrames / std::chrono::duration<double>(Clock::now() - start).count());
    }
    // Temps GPU d'une image isolee (evenements CUDA, comme le spike : « ms_event_median ») : separe
    // le cout GPU de l'effet du cout cote CPU (pont, contexte, verrou) et de la charge de la carte.
    std::vector<float> gpu_ms;
    cudaEvent_t ev0 = nullptr;
    cudaEvent_t ev1 = nullptr;
    ASSERT_EQ(cudaEventCreate(&ev0), cudaSuccess);
    ASSERT_EQ(cudaEventCreate(&ev1), cudaSuccess);
    for (int i = 0; i < 30; ++i) {
        ASSERT_EQ(cudaEventRecord(ev0, stream()), cudaSuccess);
        ASSERT_TRUE((*up)->upscale(request(inputs[0], *out, UpscaleQuality::Ultra)));
        ASSERT_EQ(cudaEventRecord(ev1, stream()), cudaSuccess);
        ASSERT_EQ(cudaEventSynchronize(ev1), cudaSuccess);
        float ms = 0.0F;
        ASSERT_EQ(cudaEventElapsedTime(&ms, ev0, ev1), cudaSuccess);
        gpu_ms.push_back(ms);
    }
    cudaEventDestroy(ev0);
    cudaEventDestroy(ev1);
    std::ranges::sort(gpu_ms);
    std::cout << "[ENH-040] VSR q4 " << bitsName(fmt)
              << " : temps GPU d'une image isolee (mediane) " << gpu_ms[gpu_ms.size() / 2]
              << " ms\n";

    std::ranges::sort(fps);
    const double median = fps[kRepeats / 2];
    const double spread = (fps.back() - fps.front()) / median;
    std::cout << "[ENH-040] VSR q4 " << bitsName(fmt) << " 720p->4K : mediane " << median
              << " i/s (min " << fps.front() << ", max " << fps.back() << ", ecart "
              << spread * 100.0 << " %)" << (spread > 0.03 ? " INSTABLE" : "") << '\n';
    RecordProperty("fps_median", std::to_string(median));
    if (median < 300.0) {
        std::cout << "[ENH-040] AVERTISSEMENT : sous le budget de 300 i/s (QA : 240-300)\n";
    }
    EXPECT_GE(median, 240.0) << "FAIL QA (< 240 i/s)";
}

TEST_F(ENH040, Vram_EffetQ4_SousLeBudget) {
    const ProcessVram vram;
    ASSERT_TRUE(vram.ok());
    const double rest = vram.mib();
    auto in = CudaRgbImage::create(kIn, RgbFormat::Abgr10);
    auto out = CudaRgbImage::create(kOut, RgbFormat::Abgr10);
    ASSERT_TRUE(in && out);
    ASSERT_TRUE(in->upload(syntheticImage(kIn, RgbFormat::Abgr10), stream()));
    ASSERT_EQ(cudaStreamSynchronize(stream()), cudaSuccess);
    const double before_effect = vram.mib();
    double peak = 0.0;
    {
        PeakSampler sampler(vram);
        auto up = makeUpscaler();
        ASSERT_TRUE(up) << up.error().detail;
        for (int i = 0; i < 60; ++i) {
            ASSERT_TRUE((*up)->upscale(request(*in, *out, UpscaleQuality::Ultra)));
        }
        ASSERT_EQ(cudaStreamSynchronize(stream()), cudaSuccess);
        peak = sampler.stop();
    }
    const double effect = peak - before_effect;
    std::cout << "[ENH-040] VRAM processus : tampons E/S " << before_effect - rest
              << " Mio, effet q4 10 bits (pic - avant creation) " << effect << " Mio\n";
    RecordProperty("vram_effect_mib", std::to_string(effect));
    EXPECT_LE(effect, 300.0);
}

TEST_F(ENH040, DixCycles_CreationDestruction_SansDerive) {
    const ProcessVram vram;
    ASSERT_TRUE(vram.ok());
    auto in = CudaRgbImage::create(kIn, RgbFormat::Argb8);
    auto out = CudaRgbImage::create(kOut, RgbFormat::Argb8);
    ASSERT_TRUE(in && out);
    ASSERT_TRUE(in->upload(syntheticImage(kIn, RgbFormat::Argb8), stream()));
    const auto cycle = [&] {
        auto up = makeUpscaler(); // runtime NGX + effet (init / arret de NGX a chaque cycle)
        ASSERT_TRUE(up) << up.error().detail;
        for (int i = 0; i < 5; ++i) {
            ASSERT_TRUE((*up)->upscale(request(*in, *out, UpscaleQuality::Ultra)));
        }
        ASSERT_EQ(cudaStreamSynchronize(stream()), cudaSuccess);
    };
    cycle(); // premier cycle : initialisations paresseuses du pilote, non compte
    std::vector<double> mib;
    std::vector<DWORD> handles;
    for (int i = 0; i < 10; ++i) {
        cycle();
        if (HasFatalFailure()) {
            return;
        }
        mib.push_back(vram.mib());
        handles.push_back(handleCount());
    }
    std::cout << "[ENH-040] 10 cycles, VRAM processus apres chaque cycle (Mio) :";
    for (const double v : mib) {
        std::cout << ' ' << v;
    }
    std::cout << "\n[ENH-040] handles :";
    for (const DWORD h : handles) {
        std::cout << ' ' << h;
    }
    std::cout << '\n';
    const double drift = mib.back() - mib.front();
    const long handle_drift =
        static_cast<long>(handles.back()) - static_cast<long>(handles.front());
    RecordProperty("vram_drift_mib", std::to_string(drift));
    RecordProperty("handle_drift", std::to_string(handle_drift));
    EXPECT_LE(std::abs(drift), 50.0) << "seuil QA";
    EXPECT_LE(std::abs(drift), 2.0) << "budget de la note GPU (0 Mio, tolerance 2)";
    EXPECT_EQ(handle_drift, 0);
}

INSTANTIATE_TEST_SUITE_P(Formats, ENH040Bits,
                         ::testing::Values(RgbFormat::Argb8, RgbFormat::Abgr10),
                         [](const ::testing::TestParamInfo<RgbFormat>& p) {
                             return p.param == RgbFormat::Argb8 ? std::string("Bits8")
                                                                : std::string("Bits10");
                         });

} // namespace
} // namespace enh::pipeline
