// ENH-040 sans GPU (CI) : RAII du pont (ordre init -> create -> destroy -> shutdown, liberation sur
// erreur, deplacement), traduction des codes NGX, chargement de DLL (vraie DLL factice), controle
// des modules et des chemins, signature, contrat IUpscaler (faux et vrai upscaler sur faux pont).

#include "enh/pipeline/core/log.hpp"
#include "enh/pipeline/rtxvideo/bridge_library.hpp"
#include "enh/pipeline/rtxvideo/module_trust.hpp"
#include "enh/pipeline/rtxvideo/ngx_runtime.hpp"
#include "enh/pipeline/rtxvideo/rtxvideo_upscaler.hpp"

#include "fakes/fake_bridge.hpp"
#include "fakes/fake_upscaler.hpp"
#include "pe/pe_image.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace enh::pipeline {
namespace {

namespace fs = std::filesystem;
using fakes::fakeBridge;
using fakes::fakeBridgeApi;
using rtxvideo::BridgeLibrary;
using rtxvideo::NgxRuntime;
using rtxvideo::NgxRuntimeOptions;
using rtxvideo::RtxVideoUpscaler;
using rtxvideo::VsrEffect;

int g_context_tag = 0; // adresses factices, jamais dereferencees par le faux pont
int g_stream_tag = 0;
void* const kContext = &g_context_tag;
void* const kStream = &g_stream_tag;

fs::path tempDir(const char* name) {
    const fs::path p = fs::temp_directory_path() / "enh_rtxvideo_tests" / name;
    std::error_code ec;
    fs::remove_all(p, ec);
    fs::create_directories(p);
    return p;
}

NgxRuntimeOptions options() {
    NgxRuntimeOptions o;
    o.data_dir = fs::temp_directory_path() / "enh_rtxvideo_tests" / "ngx";
    o.cu_context = kContext;
    return o;
}

UpscaleRequest validRequest() {
    UpscaleRequest r;
    r.input_texture = 11;
    r.input_size_px = {.width = 1280, .height = 720};
    r.output_surface = 22;
    r.output_size_px = {.width = 3840, .height = 2160};
    r.quality = UpscaleQuality::Ultra;
    return r;
}

class RtxVideoUnit : public ::testing::Test {
protected:
    void SetUp() override { fakes::resetFakeBridge(); }
};

// --- Traduction des codes -------------------------------------------------------------------

TEST(RtxVideoErrors, CodesNgxTraduits) {
    using rtxvideo::bridgeFailure;
    const auto check = [](rvb_result r, PipelineErrorCode code, Component comp) {
        const PipelineError e = bridgeFailure(r, "test");
        EXPECT_EQ(e.code, code) << std::hex << r;
        EXPECT_EQ(e.component, comp) << std::hex << r;
        EXPECT_EQ(e.native_code, static_cast<std::int64_t>(r));
    };
    check(0xBAD00001U, PipelineErrorCode::ComponentMissing, Component::RtxVideoSdk);
    check(0xBAD00004U, PipelineErrorCode::ComponentMissing, Component::RtxVideoSdk);
    check(0xBAD0000CU, PipelineErrorCode::ComponentMissing, Component::Driver);
    check(0xBAD0000DU, PipelineErrorCode::OutOfVram, Component::None);
    check(0xBAD00005U, PipelineErrorCode::InvalidParams, Component::None);
    check(0xBAD0000EU, PipelineErrorCode::InvalidParams, Component::None);
    check(0xBAD0000FU, PipelineErrorCode::WriteDenied, Component::None);
    check(0xBAD00000U, PipelineErrorCode::Internal, Component::None);
    check(RVB_E_ALREADY_INITIALIZED, PipelineErrorCode::Busy, Component::None);
    check(RVB_E_INVALID_ARG, PipelineErrorCode::InvalidParams, Component::None);
    check(RVB_E_INTERNAL, PipelineErrorCode::Internal, Component::None);
}

TEST(RtxVideoErrors, SuccesEtNoms) {
    EXPECT_TRUE(rtxvideo::succeeded(RVB_OK));
    EXPECT_TRUE(rtxvideo::succeeded(0x2U)); // autre code de succes NGX eventuel
    EXPECT_FALSE(rtxvideo::succeeded(0xBAD0000DU));
    EXPECT_FALSE(rtxvideo::succeeded(RVB_E_BUSY));
    EXPECT_EQ(rtxvideo::resultName(0xBAD0000DU), "FAIL_OutOfGPUMemory");
    EXPECT_EQ(rtxvideo::resultName(RVB_E_BUSY), "RVB_E_BUSY");
    EXPECT_EQ(rtxvideo::resultName(0xBAD000FFU), "FAIL_Unknown");
    const PipelineError e = rtxvideo::bridgeFailure(0xBAD0000DU, "rvb_vsr_create");
    EXPECT_NE(e.detail.find("0xBAD0000D"), std::string::npos);
    EXPECT_NE(e.detail.find("FAIL_OutOfGPUMemory"), std::string::npos);
}

// --- NgxRuntime / VsrEffect (RAII) -------------------------------------------------------------

TEST_F(RtxVideoUnit, Runtime_InitPuisArret_ParametresSec01) {
    {
        auto rt = NgxRuntime::create(fakeBridgeApi(), options());
        ASSERT_TRUE(rt) << rt.error().detail;
        EXPECT_EQ(fakeBridge().live_runtimes, 1);
        EXPECT_EQ(fakeBridge().init_struct_size, sizeof(rvb_init_params));
        EXPECT_EQ(fakeBridge().init_log_level, RVB_LOG_ON); // lu en memoire seulement
        EXPECT_TRUE(fakeBridge().init_has_log_fn);          // observateur toujours branche
        EXPECT_EQ(fakeBridge().init_context, kContext);
        EXPECT_EQ(fs::path(fakeBridge().init_data_path), options().data_dir);
        EXPECT_TRUE(fs::is_directory(options().data_dir)); // dossier cree
    }
    EXPECT_EQ(fakeBridge().live_runtimes, 0);
    const std::vector<std::string> expected = {"runtime_create", "runtime_destroy"};
    EXPECT_EQ(fakeBridge().calls, expected);
}

TEST_F(RtxVideoUnit, Runtime_JournalDemande_RappelFourni) {
    NgxRuntimeOptions o = options();
    o.log_level = rtxvideo::NgxLogLevel::Verbose;
    auto rt = NgxRuntime::create(fakeBridgeApi(), o);
    ASSERT_TRUE(rt);
    EXPECT_EQ(fakeBridge().init_log_level, RVB_LOG_VERBOSE);
    EXPECT_TRUE(fakeBridge().init_has_log_fn);
}

TEST_F(RtxVideoUnit, Runtime_ParametresManquants_InvalidParams) {
    NgxRuntimeOptions o = options();
    o.cu_context = nullptr;
    auto rt = NgxRuntime::create(fakeBridgeApi(), o);
    ASSERT_FALSE(rt);
    EXPECT_EQ(rt.error().code, PipelineErrorCode::InvalidParams);
    EXPECT_TRUE(fakeBridge().calls.empty());

    rtxvideo::BridgeApi incomplete = fakeBridgeApi();
    incomplete.vsr_evaluate = nullptr;
    auto rt2 = NgxRuntime::create(incomplete, options());
    ASSERT_FALSE(rt2);
    EXPECT_EQ(rt2.error().code, PipelineErrorCode::Internal);
}

TEST_F(RtxVideoUnit, Runtime_EchecInit_RienALiberer) {
    fakeBridge().runtime_create_result = 0xBAD0000FU; // dossier de donnees non inscriptible
    auto rt = NgxRuntime::create(fakeBridgeApi(), options());
    ASSERT_FALSE(rt);
    EXPECT_EQ(rt.error().code, PipelineErrorCode::WriteDenied);
    const std::vector<std::string> expected = {"runtime_create"};
    EXPECT_EQ(fakeBridge().calls, expected);
}

TEST_F(RtxVideoUnit, Runtime_UnSeulParProcessus_Busy) {
    auto first = NgxRuntime::create(fakeBridgeApi(), options());
    ASSERT_TRUE(first);
    auto second = NgxRuntime::create(fakeBridgeApi(), options());
    ASSERT_FALSE(second);
    EXPECT_EQ(second.error().code, PipelineErrorCode::Busy);
}

TEST_F(RtxVideoUnit, Runtime_Deplacement_PasDeDoubleLiberation) {
    {
        auto rt = NgxRuntime::create(fakeBridgeApi(), options());
        ASSERT_TRUE(rt);
        NgxRuntime moved = std::move(*rt);
        NgxRuntime assigned = std::move(moved);
        EXPECT_NE(assigned.handle(), nullptr);
    }
    EXPECT_EQ(std::ranges::count(fakeBridge().calls, "runtime_destroy"), 1);
    EXPECT_EQ(fakeBridge().live_runtimes, 0);
}

TEST_F(RtxVideoUnit, Effet_OrdreCreationDestruction) {
    {
        auto rt = NgxRuntime::create(fakeBridgeApi(), options());
        ASSERT_TRUE(rt);
        auto fx = VsrEffect::create(*rt, kContext, kStream);
        ASSERT_TRUE(fx) << fx.error().detail;
        EXPECT_EQ(fakeBridge().create_stream, kStream);
        VsrEffect moved = std::move(*fx);
        EXPECT_TRUE(moved.evaluate(validRequest()));
    } // effet detruit avant le runtime (ordre inverse des declarations)
    const std::vector<std::string> expected = {"runtime_create", "vsr_create", "effect_destroy",
                                               "runtime_destroy"};
    EXPECT_EQ(fakeBridge().calls, expected);
    EXPECT_EQ(fakeBridge().live_effects, 0);
    EXPECT_EQ(fakeBridge().live_runtimes, 0);
}

TEST_F(RtxVideoUnit, Effet_EchecCreation_OutOfVram_RuntimeLibere) {
    fakeBridge().vsr_create_result = 0xBAD0000DU;
    {
        auto rt = NgxRuntime::create(fakeBridgeApi(), options());
        ASSERT_TRUE(rt);
        auto fx = VsrEffect::create(*rt, kContext, kStream);
        ASSERT_FALSE(fx);
        EXPECT_EQ(fx.error().code, PipelineErrorCode::OutOfVram);
    }
    EXPECT_EQ(fakeBridge().live_runtimes, 0);
    EXPECT_EQ(std::ranges::count(fakeBridge().calls, "effect_destroy"), 0);
}

TEST_F(RtxVideoUnit, Effet_ParametresTransmisAuPont) {
    auto rt = NgxRuntime::create(fakeBridgeApi(), options());
    ASSERT_TRUE(rt);
    auto fx = VsrEffect::create(*rt, kContext, kStream);
    ASSERT_TRUE(fx);
    UpscaleRequest req = validRequest();
    req.quality = UpscaleQuality::Bicubic;
    ASSERT_TRUE(fx->evaluate(req));
    const rvb_vsr_eval_params& p = fakeBridge().last_eval;
    EXPECT_EQ(p.struct_size, sizeof(rvb_vsr_eval_params));
    EXPECT_EQ(p.quality, 0U);
    EXPECT_EQ(p.input_texture, 11U);
    EXPECT_EQ(p.output_surface, 22U);
    EXPECT_EQ(p.input_rect.width, 1280U);
    EXPECT_EQ(p.input_rect.height, 720U);
    EXPECT_EQ(p.output_rect.width, 3840U);
    EXPECT_EQ(p.output_rect.height, 2160U);
    EXPECT_EQ(p.output_rect.x, 0U);
}

TEST_F(RtxVideoUnit, Effet_ErreurEvaluation_Traduite) {
    auto rt = NgxRuntime::create(fakeBridgeApi(), options());
    ASSERT_TRUE(rt);
    auto fx = VsrEffect::create(*rt, kContext, kStream);
    ASSERT_TRUE(fx);
    fakeBridge().evaluate_result = 0xBAD00008U; // format d'entree refuse
    auto r = fx->evaluate(validRequest());
    ASSERT_FALSE(r);
    EXPECT_EQ(r.error().code, PipelineErrorCode::InvalidParams);
}

// --- RtxVideoUpscaler sur faux pont ------------------------------------------------------------

TEST_F(RtxVideoUnit, Upscaler_VsrIndisponible_ComponentMissing) {
    fakeBridge().caps.available = 0;
    auto up = RtxVideoUpscaler::createWithApi(fakeBridgeApi(), options(), kStream);
    ASSERT_FALSE(up);
    EXPECT_EQ(up.error().code, PipelineErrorCode::ComponentMissing);
    EXPECT_EQ(up.error().component, Component::RtxVideoSdk);
    EXPECT_EQ(fakeBridge().live_runtimes, 0); // runtime libere sur le chemin d'erreur
}

TEST_F(RtxVideoUnit, Upscaler_PiloteTropAncien_ComponentMissingDriver) {
    fakeBridge().caps.available = 0;
    fakeBridge().caps.needs_updated_driver = 1;
    fakeBridge().caps.min_driver_major = 570;
    fakeBridge().caps.min_driver_minor = 20;
    auto up = RtxVideoUpscaler::createWithApi(fakeBridgeApi(), options(), kStream);
    ASSERT_FALSE(up);
    EXPECT_EQ(up.error().component, Component::Driver);
    EXPECT_NE(up.error().detail.find("570.20"), std::string::npos);
}

TEST_F(RtxVideoUnit, Upscaler_CycleCompletEtLiberation) {
    for (int i = 0; i < 10; ++i) { // 10 creations / destructions : rien ne reste
        auto up = RtxVideoUpscaler::createWithApi(fakeBridgeApi(), options(), kStream);
        ASSERT_TRUE(up) << up.error().detail;
        EXPECT_TRUE((*up)->capabilities().available);
        ASSERT_TRUE((*up)->upscale(validRequest()));
    }
    EXPECT_EQ(fakeBridge().live_effects, 0);
    EXPECT_EQ(fakeBridge().live_runtimes, 0);
    EXPECT_EQ(fakeBridge().evaluations, 10);
}

// --- Contrat IUpscaler : meme comportement pour le faux et pour le vrai (sur faux pont) --------

void expectUpscalerContract(IUpscaler& up) {
    EXPECT_TRUE(up.upscale(validRequest()));
    UpscaleRequest bad = validRequest();
    bad.input_texture = 0;
    auto r = up.upscale(bad);
    ASSERT_FALSE(r);
    EXPECT_EQ(r.error().code, PipelineErrorCode::InvalidParams);
    bad = validRequest();
    bad.output_size_px = {.width = 0, .height = 2160};
    EXPECT_FALSE(up.upscale(bad));
    bad = validRequest();
    bad.quality = static_cast<UpscaleQuality>(5);
    EXPECT_FALSE(up.upscale(bad));
}

TEST_F(RtxVideoUnit, ContratIUpscaler_FakeUpscaler) {
    fakes::FakeUpscaler fake;
    expectUpscalerContract(fake);
    EXPECT_EQ(fake.calls(), 1);
    fakes::FakeUpscaler failing(0, PipelineErrorCode::OutOfVram);
    auto r = failing.upscale(validRequest());
    ASSERT_FALSE(r);
    EXPECT_EQ(r.error().code, PipelineErrorCode::OutOfVram);
}

TEST_F(RtxVideoUnit, ContratIUpscaler_RtxVideoUpscaler) {
    auto up = RtxVideoUpscaler::createWithApi(fakeBridgeApi(), options(), kStream);
    ASSERT_TRUE(up);
    expectUpscalerContract(**up);
    EXPECT_EQ(fakeBridge().evaluations, 1); // les requetes invalides n'atteignent pas le pont
}

// --- BridgeLibrary (vraie DLL factice) ---------------------------------------------------------

fs::path testDir() {
    auto dir = rtxvideo::applicationDirectory();
    EXPECT_TRUE(dir);
    return dir ? *dir : fs::path{};
}

TEST(RtxVideoLibrary, CheminRelatif_InvalidParams) {
    auto lib = BridgeLibrary::load(L"enh_rtxvideo_bridge.dll");
    ASSERT_FALSE(lib);
    EXPECT_EQ(lib.error().code, PipelineErrorCode::InvalidParams);
}

TEST(RtxVideoLibrary, DllAbsente_ComponentMissing) {
    auto lib = BridgeLibrary::load(testDir() / L"absent_enh_bridge.dll");
    ASSERT_FALSE(lib);
    EXPECT_EQ(lib.error().code, PipelineErrorCode::ComponentMissing);
    EXPECT_EQ(lib.error().component, Component::RtxVideoSdk);
    EXPECT_EQ(lib.error().native_code, ERROR_MOD_NOT_FOUND);
}

TEST(RtxVideoLibrary, DllSansExports_ComponentMissing) {
    std::array<wchar_t, MAX_PATH> sys{};
    ASSERT_GT(GetSystemDirectoryW(sys.data(), MAX_PATH), 0U);
    auto lib = BridgeLibrary::load(fs::path(sys.data()) / L"version.dll");
    ASSERT_FALSE(lib);
    EXPECT_EQ(lib.error().code, PipelineErrorCode::ComponentMissing);
    EXPECT_NE(lib.error().detail.find("missing export"), std::string::npos);
}

TEST(RtxVideoLibrary, DllFactice_AbiOk_Chargee) {
    auto lib = BridgeLibrary::load(testDir() / ENH_FAKE_BRIDGE_OK);
    ASSERT_TRUE(lib) << lib.error().detail;
    EXPECT_TRUE(lib->api().complete());
    EXPECT_EQ(lib->api().abi_version(), RVB_ABI_VERSION);
    BridgeLibrary moved = std::move(*lib);
    EXPECT_TRUE(moved.api().complete());
}

TEST(RtxVideoLibrary, DllFactice_AbiDifferente_Refusee) {
    auto lib = BridgeLibrary::load(testDir() / ENH_FAKE_BRIDGE_BAD_ABI);
    ASSERT_FALSE(lib);
    EXPECT_EQ(lib.error().code, PipelineErrorCode::ComponentMissing);
    EXPECT_EQ(lib.error().native_code, 99);
}

TEST(RtxVideoLibrary, DllFactice_ExportManquant_Refusee) {
    auto lib = BridgeLibrary::load(testDir() / ENH_FAKE_BRIDGE_PARTIAL);
    ASSERT_FALSE(lib);
    EXPECT_EQ(lib.error().code, PipelineErrorCode::ComponentMissing);
}

TEST(RtxVideoLibrary, DllFactice_DechargeeALaDestruction) {
    const fs::path p = testDir() / ENH_FAKE_BRIDGE_OK;
    {
        auto lib = BridgeLibrary::load(p);
        ASSERT_TRUE(lib);
        EXPECT_NE(GetModuleHandleW(p.c_str()), nullptr);
    }
    EXPECT_EQ(GetModuleHandleW(p.c_str()), nullptr);
}

// --- Chemins et modules ------------------------------------------------------------------------

TEST(RtxVideoModules, CleLexicale_CassePrefixesSegments) {
    using rtxvideo::lexicalPathKey;
    EXPECT_EQ(lexicalPathKey(L"C:\\App\\nvngx_vsr.dll"), lexicalPathKey(L"c:/app/NVNGX_VSR.DLL"));
    EXPECT_EQ(lexicalPathKey(L"\\\\?\\C:\\App\\x.dll"), lexicalPathKey(L"C:\\App\\x.dll"));
    EXPECT_EQ(lexicalPathKey(L"C:\\App\\sub\\..\\.\\x.dll"), lexicalPathKey(L"C:\\App\\x.dll"));
    EXPECT_EQ(lexicalPathKey(L"\\\\?\\UNC\\srv\\share\\x.dll"),
              lexicalPathKey(L"\\\\srv\\share\\x.dll"));
    EXPECT_NE(lexicalPathKey(L"C:\\App\\x.dll"), lexicalPathKey(L"C:\\Other\\x.dll"));
}

TEST(RtxVideoModules, MemeFichier_ParCheminFinal) {
    const fs::path dir = tempDir("samefile");
    const fs::path f = dir / L"Feature.dll";
    std::ofstream(f) << "x";
    const fs::path other = dir / L"Other.dll";
    std::ofstream(other) << "y";
    std::wstring upper = f.wstring();
    std::ranges::transform(upper, upper.begin(), ::towupper);
    EXPECT_TRUE(rtxvideo::sameFile(f, fs::path(upper)));
    EXPECT_TRUE(rtxvideo::sameFile(f, fs::path(L"\\\\?\\" + f.wstring())));
    EXPECT_TRUE(rtxvideo::sameFile(f, dir / L"." / L"Feature.dll"));
    EXPECT_FALSE(rtxvideo::sameFile(f, other));
}

TEST(RtxVideoModules, ModulesCharges_Kernel32DansSystem32) {
    const auto loaded = rtxvideo::loadedModulePaths(L"KERNEL32.DLL");
    ASSERT_EQ(loaded.size(), 1U);
    std::array<wchar_t, MAX_PATH> sys{};
    ASSERT_GT(GetSystemDirectoryW(sys.data(), MAX_PATH), 0U);
    EXPECT_TRUE(rtxvideo::sameFile(loaded.front(), fs::path(sys.data()) / L"kernel32.dll"));
    EXPECT_TRUE(rtxvideo::loadedModulePaths(L"nvngx_vsr.dll").empty());
}

TEST(RtxVideoModules, MemeContenu_OctetParOctet) {
    const fs::path dir = tempDir("content");
    std::ofstream(dir / L"a.dll", std::ios::binary) << std::string(3 << 20, 'x');
    std::ofstream(dir / L"b.dll", std::ios::binary) << std::string(3 << 20, 'x');
    std::string almost(3 << 20, 'x');
    almost.back() = 'y';
    std::ofstream(dir / L"c.dll", std::ios::binary) << almost;
    std::ofstream(dir / L"d.dll", std::ios::binary) << "short";
    EXPECT_TRUE(*rtxvideo::sameContent(dir / L"a.dll", dir / L"b.dll"));
    EXPECT_FALSE(*rtxvideo::sameContent(dir / L"a.dll", dir / L"c.dll"));
    EXPECT_FALSE(*rtxvideo::sameContent(dir / L"a.dll", dir / L"d.dll"));
    auto missing = rtxvideo::sameContent(dir / L"a.dll", dir / L"absent.dll");
    ASSERT_FALSE(missing);
    EXPECT_EQ(missing.error().code, PipelineErrorCode::SourceUnreadable);
}

// --- Garde-fous NGX (constates au pilote 617.42) -----------------------------------------------

TEST(RtxVideoNgxGuard, JournalNgx_DllDeFonctionLue) {
    rtxvideo::NgxLogObserver obs;
    obs.onLine(R"([2026-10-23 19:56:51] [NGXSecureLoadFeature:1375] Found vsr snippet at C:\X)");
    EXPECT_TRUE(obs.vsrSnippet().path.empty());
    obs.onLine(R"([2026-10-23 19:56:51] [NGXSecureLoadFeature:1499] app E658703 feature vsr )"
               R"(snippet: C:\ProgramData\NVIDIA\NGX\models\vsr\versions\67072\files/)"
               "160_E658703.bin version: 1.6.0\n");
    const auto s = obs.vsrSnippet();
    EXPECT_EQ(s.version, "1.6.0");
    EXPECT_EQ(s.path, fs::path(LR"(C:\ProgramData\NVIDIA\NGX\models\vsr\versions\67072\files)"
                               LR"(\160_E658703.bin)"));
}

TEST_F(RtxVideoUnit, JournalNgx_LuEnMemoireViaLePont) {
    fakeBridge().log_line_on_vsr_create =
        R"([x] [NGXSecureLoadFeature:1499] app E658703 feature vsr snippet: D:\app/nvngx_vsr.dll )"
        "version: 1.6.0";
    auto rt = NgxRuntime::create(fakeBridgeApi(), options());
    ASSERT_TRUE(rt);
    EXPECT_EQ(fakeBridge().init_log_level, RVB_LOG_ON); // lu en memoire, pas renvoye au journal
    auto fx = VsrEffect::create(*rt, kContext, kStream);
    ASSERT_TRUE(fx);
    EXPECT_EQ(rt->vsrSnippet().path, fs::path(LR"(D:\app\nvngx_vsr.dll)"));
    EXPECT_EQ(rt->vsrSnippet().version, "1.6.0");
}

TEST(RtxVideoNgxGuard, DllDeFonction_OrigineVerifiee_DecisionPo) {
    const fs::path dir = tempDir("snippet");
    const fs::path shipped = dir / L"app" / L"nvngx_vsr.dll";
    const fs::path store_same = dir / L"store" / L"160_E658703.bin";
    const fs::path store_other = dir / L"store" / L"170_E658703.bin";
    fs::create_directories(shipped.parent_path());
    fs::create_directories(store_same.parent_path());
    std::ofstream(shipped, std::ios::binary) << "snippet v1.6.0";
    std::ofstream(store_same, std::ios::binary) << "snippet v1.6.0";
    std::ofstream(store_other, std::ios::binary) << "snippet v1.7.0";

    // Signature simulee (aucun fichier signe NVIDIA en CI).
    const auto signedBy = [](const std::wstring& who) {
        rtxvideo::SnippetCheckers c;
        // NOLINTNEXTLINE(bugprone-exception-escape) : faux de test, copie de chaine
        c.signer = [who](const fs::path&) -> Result<std::wstring> { return who; };
        c.fingerprint = [](const fs::path&) -> Result<std::string> { return std::string("ab12"); };
        return c;
    };
    rtxvideo::SnippetCheckers unsigned_file;
    unsigned_file.signer = [](const fs::path&) -> Result<std::wstring> {
        return fail(PipelineErrorCode::ComponentMissing, "not signed", 0x800B0100);
    };

    auto app = rtxvideo::verifyVsrSnippet({.path = shipped, .version = "1.6.0"}, shipped);
    ASSERT_TRUE(app);
    EXPECT_EQ(app->source, rtxvideo::SnippetSource::AppFolder);

    auto same = rtxvideo::verifyVsrSnippet({.path = store_same, .version = "1.6.0"}, shipped);
    ASSERT_TRUE(same);
    EXPECT_EQ(same->source, rtxvideo::SnippetSource::NgxStoreIdentical);

    // Autre DLL signee NVIDIA : acceptee, avertissement au journal (version + empreinte, sans
    // chemin).
    std::vector<std::string> warnings;
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) : signature imposee par LogSink
    setLogSink([&](LogLevel level, std::string_view code, std::string_view msg) {
        if (level == LogLevel::Warn && code == "VsrFeatureNotShipped") {
            warnings.emplace_back(msg);
        }
    });
    auto nvidia = rtxvideo::verifyVsrSnippet({.path = store_other, .version = "1.7.0"}, shipped,
                                             signedBy(L"NVIDIA Corporation"));
    setLogSink({});
    ASSERT_TRUE(nvidia);
    EXPECT_EQ(nvidia->source, rtxvideo::SnippetSource::NvidiaSignedOther);
    EXPECT_EQ(nvidia->fingerprint, "ab12");
    ASSERT_EQ(warnings.size(), 1U);
    EXPECT_NE(warnings.front().find("1.7.0"), std::string::npos);
    EXPECT_NE(warnings.front().find("ab12"), std::string::npos);
    EXPECT_EQ(warnings.front().find("store"), std::string::npos); // aucun chemin

    // Autre signataire, ou non signee : refus, erreur distincte et stable.
    for (const auto& checkers : {signedBy(L"Contoso Ltd"), unsigned_file}) {
        auto r = rtxvideo::verifyVsrSnippet({.path = store_other, .version = "1.7.0"}, shipped,
                                            checkers);
        ASSERT_FALSE(r);
        EXPECT_EQ(r.error().code, PipelineErrorCode::ComponentMissing);
        EXPECT_EQ(r.error().component, Component::RtxVideoFeatureUntrusted);
        EXPECT_TRUE(r.error().detail.starts_with("VsrFeatureUntrusted: "));
        EXPECT_EQ(r.error().detail.find("store"), std::string::npos);
    }

    // NGX n'annonce pas sa DLL : refus.
    auto silent = rtxvideo::verifyVsrSnippet({}, shipped, signedBy(L"NVIDIA Corporation"));
    ASSERT_FALSE(silent);
    EXPECT_EQ(silent.error().component, Component::RtxVideoFeatureUntrusted);
}

TEST(RtxVideoModules, EmpreinteAuthenticode_Sha256) {
    std::array<wchar_t, MAX_PATH> sys{};
    ASSERT_GT(GetSystemDirectoryW(sys.data(), MAX_PATH), 0U);
    auto h = rtxvideo::authenticodeSha256(fs::path(sys.data()) / L"version.dll");
    ASSERT_TRUE(h) << h.error().detail;
    EXPECT_EQ(h->size(), 64U);
    auto again = rtxvideo::authenticodeSha256(fs::path(sys.data()) / L"version.dll");
    ASSERT_TRUE(again);
    EXPECT_EQ(*h, *again);
    EXPECT_FALSE(rtxvideo::authenticodeSha256(tempDir("fp") / L"absent.dll"));
}

/// Lance `cmd.exe /c exit 0` ; true si le processus a pu etre cree.
bool canCreateChildProcess() {
    std::array<wchar_t, MAX_PATH> sys{};
    GetSystemDirectoryW(sys.data(), MAX_PATH);
    const std::wstring exe = (fs::path(sys.data()) / L"cmd.exe").wstring();
    std::wstring cmd = L"cmd.exe /c exit 0";
    STARTUPINFOW si{};
    si.cb = sizeof si;
    PROCESS_INFORMATION pi{};
    if (CreateProcessW(exe.c_str(), cmd.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr,
                       nullptr, &si, &pi) == FALSE) {
        return false;
    }
    WaitForSingleObject(pi.hProcess, 10000);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return true;
}

TEST(RtxVideoNgxGuard, ProcessusEnfantsBloquesPendantLaFenetreNgx) {
    ASSERT_TRUE(canCreateChildProcess());
    {
        const rtxvideo::ChildProcessBlocker blocker;
        ASSERT_TRUE(blocker.active()) << "Job Object indisponible";
        {
            const rtxvideo::ChildProcessBlocker nested; // imbrication
            EXPECT_FALSE(canCreateChildProcess());
        }
        EXPECT_FALSE(canCreateChildProcess()); // toujours bloque : un niveau reste
    }
    EXPECT_TRUE(canCreateChildProcess()); // limite levee
}

TEST(RtxVideoModules, Signature_FichierNonSigne_Refuse) {
    const fs::path f = tempDir("sig") / L"unsigned.dll";
    std::ofstream(f) << "not a signed file";
    auto r = rtxvideo::requireNvidiaSignature(f);
    ASSERT_FALSE(r);
    EXPECT_EQ(r.error().code, PipelineErrorCode::ComponentMissing);
    EXPECT_NE(r.error().native_code, 0);
}

// --- Lecteur PE (utilise par le test GPU des imports du pont) ----------------------------------

TEST(PeImage, ImportsEtExportsDUneDllSysteme) {
    std::array<wchar_t, MAX_PATH> sys{};
    ASSERT_GT(GetSystemDirectoryW(sys.data(), MAX_PATH), 0U);
    const auto info = tests::readPe(fs::path(sys.data()) / L"version.dll");
    if (!info.has_value()) {
        FAIL() << "lecture PE impossible";
    }
    const tests::PeInfo& pe = *info;
    EXPECT_FALSE(pe.imports.empty());
    const auto& exports = pe.exports;
    EXPECT_NE(std::ranges::find(exports, "GetFileVersionInfoW"), exports.end());
}

TEST(PeImage, DllFactice_ExportsRvbSeulement) {
    const auto info = tests::readPe(testDir() / ENH_FAKE_BRIDGE_OK);
    if (!info.has_value()) {
        FAIL() << "lecture PE impossible";
    }
    const tests::PeInfo& pe = *info;
    EXPECT_EQ(pe.exports.size(), 7U);
    for (const std::string& e : pe.exports) {
        EXPECT_TRUE(e.starts_with("rvb_")) << e;
    }
}

TEST(PeImage, FichierInvalide_Nullopt) {
    const fs::path f = tempDir("pe") / L"garbage.dll";
    std::ofstream(f) << "MZ but nothing else";
    EXPECT_FALSE(tests::readPe(f));
    EXPECT_FALSE(tests::readPe(f.parent_path() / L"absent.dll"));
}

} // namespace
} // namespace enh::pipeline
