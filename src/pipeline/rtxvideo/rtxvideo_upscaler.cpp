#include "enh/pipeline/rtxvideo/rtxvideo_upscaler.hpp"

#include "enh/pipeline/core/log.hpp"
#include "enh/pipeline/rtxvideo/module_trust.hpp"

#include <string>
#include <system_error>
#include <utility>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
// clang-format off
#include <windows.h>
#include <knownfolders.h>
#include <shlobj.h>
// clang-format on

namespace enh::pipeline::rtxvideo {

namespace fs = std::filesystem;

namespace {

struct CoTaskMemDeleter {
    void operator()(wchar_t* p) const noexcept { CoTaskMemFree(p); }
};

Result<VsrCapabilities> requireVsrAvailable(const NgxRuntime& runtime) {
    auto caps = runtime.queryVsr();
    if (!caps) {
        return caps;
    }
    if (caps->available) {
        return caps;
    }
    PipelineError e;
    if (caps->needs_updated_driver) {
        e = makeError(PipelineErrorCode::ComponentMissing,
                      "VSR needs driver >= " + std::to_string(caps->min_driver_major) + "." +
                          std::to_string(caps->min_driver_minor));
        e.component = Component::Driver;
    } else {
        e = makeError(PipelineErrorCode::ComponentMissing, "VSR not available on this GPU");
        e.component = Component::RtxVideoSdk;
    }
    return enh::Unexpected<PipelineError>(std::move(e));
}

} // namespace

Result<fs::path> defaultNgxDataDir() {
    wchar_t* raw = nullptr;
    const HRESULT hr = SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_DEFAULT, nullptr, &raw);
    const std::unique_ptr<wchar_t, CoTaskMemDeleter> owned(raw);
    if (FAILED(hr) || raw == nullptr) {
        return fail(PipelineErrorCode::Internal, "SHGetKnownFolderPath(LocalAppData) failed", hr);
    }
    return fs::path(raw) / L"Enhance" / L"ngx";
}

RtxVideoUpscaler::RtxVideoUpscaler(std::optional<BridgeLibrary> library, NgxRuntime runtime,
                                   VsrEffect effect, const VsrCapabilities& caps) noexcept
    : library_(std::move(library)), runtime_(std::move(runtime)), effect_(std::move(effect)),
      caps_(caps) {}

Result<std::unique_ptr<RtxVideoUpscaler>>
RtxVideoUpscaler::createWithApi(const BridgeApi& api, const NgxRuntimeOptions& options,
                                void* cu_stream) {
    return assemble(std::nullopt, api, options, cu_stream);
}

Result<std::unique_ptr<RtxVideoUpscaler>>
RtxVideoUpscaler::assemble(std::optional<BridgeLibrary> library, const BridgeApi& api,
                           const NgxRuntimeOptions& options, void* cu_stream) {
    // `library` (s'il y en a une) est un parametre : detruit APRES les objets locaux ci-dessous,
    // donc la DLL reste chargee pendant toute liberation sur un chemin d'erreur.
    auto runtime = NgxRuntime::create(api, options);
    if (!runtime) {
        return enh::Unexpected<PipelineError>(std::move(runtime.error()));
    }
    auto caps = requireVsrAvailable(*runtime);
    if (!caps) {
        return enh::Unexpected<PipelineError>(std::move(caps.error()));
    }
    auto effect = VsrEffect::create(*runtime, options.cu_context, cu_stream);
    if (!effect) {
        return enh::Unexpected<PipelineError>(std::move(effect.error()));
    }
    return std::make_unique<RtxVideoUpscaler>(std::move(library), std::move(*runtime),
                                              std::move(*effect), *caps);
}

Result<std::unique_ptr<RtxVideoUpscaler>>
RtxVideoUpscaler::create(const RtxVideoUpscalerConfig& config) {
    if (config.cu_context == nullptr) {
        return fail(PipelineErrorCode::InvalidParams, "RtxVideoUpscaler: CUDA context required");
    }
    fs::path app_dir = config.app_dir;
    if (app_dir.empty()) {
        auto dir = applicationDirectory();
        if (!dir) {
            return enh::Unexpected<PipelineError>(std::move(dir.error()));
        }
        app_dir = std::move(*dir);
    }
    fs::path data_dir = config.ngx_data_dir;
    if (data_dir.empty()) {
        auto dir = defaultNgxDataDir();
        if (!dir) {
            return enh::Unexpected<PipelineError>(std::move(dir.error()));
        }
        data_dir = std::move(*dir);
    }

    // 1. DLL de la fonction : celle livree avec l'application (copie de bin/.../rel), signee
    // NVIDIA.
    const fs::path feature_dll = app_dir / kVsrFeatureDllName;
    std::error_code ec;
    if (!fs::is_regular_file(feature_dll, ec)) {
        PipelineError e = makeError(PipelineErrorCode::ComponentMissing,
                                    "nvngx_vsr.dll missing from the application folder");
        e.component = Component::RtxVideoSdk;
        return enh::Unexpected<PipelineError>(std::move(e));
    }
    if (auto r = requireNvidiaSignature(feature_dll); !r) {
        return enh::Unexpected<PipelineError>(std::move(r.error()));
    }

    // 2. Pont, par chemin absolu.
    auto library = BridgeLibrary::load(app_dir / kBridgeDllName);
    if (!library) {
        return enh::Unexpected<PipelineError>(std::move(library.error()));
    }

    // 3 a 5.
    NgxRuntimeOptions options;
    options.data_dir = std::move(data_dir);
    options.cu_context = config.cu_context;
    options.log_level = config.log_level;
    const BridgeApi api = library->api();
    auto assembled = assemble(std::move(*library), api, options, config.cu_stream);
    if (!assembled) {
        return enh::Unexpected<PipelineError>(std::move(assembled.error()));
    }
    std::unique_ptr<RtxVideoUpscaler> upscaler = std::move(*assembled);

    // 6. Le code reellement execute est celui livre (meme fichier, ou copie identique rangee par
    //    NGX dans son magasin), et aucun processus de mise a jour n'a ete lance.
    auto provenance = verifyVsrSnippet(upscaler->runtime_.vsrSnippet(), feature_dll);
    if (!provenance) {
        return enh::Unexpected<PipelineError>(std::move(provenance.error())); // upscaler detruit
    }
    provenance->updater_blocked = upscaler->runtime_.updaterBlocked();
    provenance->child_processes_created = ChildProcessBlocker::childProcessesCreated();
    upscaler->provenance_ = std::move(*provenance);
    if (!upscaler->provenance_.updater_blocked) {
        logLine(LogLevel::Warn, "NgxUpdaterNotBlocked", "child process creation was allowed");
    }
    logLine(LogLevel::Info, "RtxVideoReady",
            std::string("VSR effect created, feature DLL ") +
                std::string(toString(upscaler->provenance_.source)) + " " +
                upscaler->provenance_.version);
    return upscaler;
}

Result<VsrProvenance> verifyVsrSnippet(const NgxSnippetInfo& snippet, const fs::path& shipped,
                                       const SnippetCheckers& checkers) {
    VsrProvenance p;
    p.version = snippet.version;
    const auto refuse = [](std::string reason) {
        PipelineError e = makeError(PipelineErrorCode::ComponentMissing,
                                    "VsrFeatureUntrusted: " + std::move(reason));
        e.component = Component::RtxVideoFeatureUntrusted; // distinct de « SDK absent »
        return enh::Unexpected<PipelineError>(std::move(e));
    };
    if (snippet.path.empty()) {
        return refuse("NGX did not report the VSR feature DLL in use");
    }
    if (sameFile(snippet.path, shipped)) {
        p.source = SnippetSource::AppFolder;
        return p;
    }
    if (auto same = sameContent(snippet.path, shipped); same && *same) {
        p.source = SnippetSource::NgxStoreIdentical;
        return p;
    }
    // Autre DLL (mise a jour NGX) : acceptee seulement si signee NVIDIA (decision PO 2026-10-24).
    auto signer = checkers.signer(snippet.path);
    if (!signer) {
        return refuse("feature DLL " + p.version + " has no valid signature (" +
                      std::to_string(signer.error().native_code) + ")");
    }
    if (*signer != kNvidiaSigner) {
        return refuse("feature DLL " + p.version + " is not signed by NVIDIA");
    }
    p.source = SnippetSource::NvidiaSignedOther;
    auto fingerprint = checkers.fingerprint(snippet.path);
    p.fingerprint = fingerprint ? *fingerprint : std::string("unavailable");
    logLine(LogLevel::Warn, "VsrFeatureNotShipped",
            "NVIDIA-signed VSR feature DLL " + p.version + " (not the shipped one), sha256 " +
                p.fingerprint);
    return p;
}

std::string_view toString(SnippetSource s) noexcept {
    switch (s) {
    case SnippetSource::AppFolder:
        return "AppFolder";
    case SnippetSource::NgxStoreIdentical:
        return "NgxStoreIdentical";
    case SnippetSource::NvidiaSignedOther:
        return "NvidiaSignedOther";
    case SnippetSource::Untrusted:
        return "Untrusted";
    case SnippetSource::Unknown:
        break;
    }
    return "Unknown";
}

Result<void> RtxVideoUpscaler::upscale(const UpscaleRequest& request) noexcept {
    return effect_.evaluate(request);
}

} // namespace enh::pipeline::rtxvideo
