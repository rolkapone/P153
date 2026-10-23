#pragma once

// IUpscaler du RTX Video SDK (docs/gpu/ENH-040.md). Assemble, dans cet ordre :
//   1. nvngx_vsr.dll du dossier de l'application : presente et signee NVIDIA (hors ligne) ;
//   2. enh_rtxvideo_bridge.dll chargee par chemin absolu, ABI verifiee ;
//   3. runtime NGX (identifiant d'application 0, journal coupe par defaut) ;
//   4. capacites VSR (disponible ? pilote assez recent ?) ;
//   5. effet VSR sur le contexte / flux CUDA du pipeline ;
//   6. la DLL de fonction que NGX dit executer = nvngx_vsr.dll livree (meme fichier ou copie
//      identique octet pour octet dans le magasin NGX), sinon refus (require_shipped_snippet).
// Les etapes 3 et 5 se font processus enfants interdits (pas de nvngx_update.exe, ngx_guard.hpp).
// Tout echec rend une PipelineError (aucune exception, aucun plantage) et libere ce qui a ete cree.

#include "enh/pipeline/rtxvideo/bridge_library.hpp"
#include "enh/pipeline/rtxvideo/ngx_runtime.hpp"
#include "enh/pipeline/stage/upscaler.hpp"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace enh::pipeline::rtxvideo {

struct RtxVideoUpscalerConfig {
    std::filesystem::path app_dir;      // pont + nvngx_vsr.dll ; vide = dossier de l'executable
    std::filesystem::path ngx_data_dir; // vide = %LOCALAPPDATA%\Enhance\ngx
    void* cu_context = nullptr;         // CUcontext (obligatoire)
    void* cu_stream = nullptr;          // CUstream du traitement
    NgxLogLevel log_level = NgxLogLevel::Off;
    /// true : refuser (ComponentMissing) si NGX execute une DLL de fonction dont le contenu
    /// differe de nvngx_vsr.dll livree, ou s'il ne dit pas laquelle il execute (SEC-01).
    bool require_shipped_snippet = true;
};

/// Origine de la DLL de fonction VSR reellement executee par NGX.
enum class SnippetSource : std::uint8_t {
    Unknown,           // NGX ne l'a pas annoncee dans son journal
    AppFolder,         // nvngx_vsr.dll du dossier de l'application
    NgxStoreIdentical, // copie rangee par NGX dans %ProgramData%\NVIDIA\NGX, octet pour octet
    Foreign,           // autre contenu (mise a jour NGX) : refuse par defaut
};
[[nodiscard]] std::string_view toString(SnippetSource s) noexcept;

struct VsrProvenance {
    SnippetSource source = SnippetSource::Unknown;
    std::string version;                       // version annoncee par NGX (ex. "1.6.0")
    bool updater_blocked = false;              // processus enfants interdits pendant l'init
    std::uint32_t child_processes_created = 0; // depuis le rattachement au Job Object (0 attendu)
};

/// Etape 6, testable sans GPU : origine de la DLL annoncee par NGX, comparee a `shipped`.
[[nodiscard]] Result<VsrProvenance> verifyVsrSnippet(const NgxSnippetInfo& snippet,
                                                     const std::filesystem::path& shipped,
                                                     bool require_shipped);

/// %LOCALAPPDATA%\Enhance\ngx (dossier de donnees NGX de l'application).
[[nodiscard]] Result<std::filesystem::path> defaultNgxDataDir();

class RtxVideoUpscaler final : public IUpscaler {
public:
    /// Chemin de production : etapes 1 a 6 ci-dessus.
    [[nodiscard]] static Result<std::unique_ptr<RtxVideoUpscaler>>
    create(const RtxVideoUpscalerConfig& config);

    /// Etapes 3 a 5 seulement, sur une table de fonctions fournie (tests sans GPU, faux pont).
    [[nodiscard]] static Result<std::unique_ptr<RtxVideoUpscaler>>
    createWithApi(const BridgeApi& api, const NgxRuntimeOptions& options, void* cu_stream);

    [[nodiscard]] Result<void> upscale(const UpscaleRequest& request) noexcept override;

    [[nodiscard]] const VsrCapabilities& capabilities() const noexcept { return caps_; }
    /// Renseignee par create() (vide avec createWithApi).
    [[nodiscard]] const VsrProvenance& provenance() const noexcept { return provenance_; }

    RtxVideoUpscaler(std::optional<BridgeLibrary> library, NgxRuntime runtime, VsrEffect effect,
                     const VsrCapabilities& caps) noexcept;

private:
    [[nodiscard]] static Result<std::unique_ptr<RtxVideoUpscaler>>
    assemble(std::optional<BridgeLibrary> library, const BridgeApi& api,
             const NgxRuntimeOptions& options, void* cu_stream);

    // Ordre de destruction inverse de la declaration : effet, puis runtime, puis DLL.
    std::optional<BridgeLibrary> library_;
    NgxRuntime runtime_;
    VsrEffect effect_;
    VsrCapabilities caps_;
    VsrProvenance provenance_;
};

} // namespace enh::pipeline::rtxvideo
