#pragma once

// RAII des objets du pont (docs/gpu/ENH-040.md § 2) :
//   NgxRuntime : rvb_runtime_create -> rvb_runtime_destroy (un seul par processus, impose par NGX)
//   VsrEffect  : rvb_vsr_create     -> rvb_effect_destroy
// Les deux copient la table BridgeApi (7 pointeurs) : la DLL doit rester chargee plus longtemps
// qu'eux (garanti par l'ordre des membres de RtxVideoUpscaler). Un VsrEffect doit etre detruit
// avant le NgxRuntime qui l'a cree (sinon le pont refuse l'arret : RVB_E_BUSY, journalise).

#include "enh/pipeline/error.hpp"
#include "enh/pipeline/rtxvideo/bridge_api.hpp"
#include "enh/pipeline/rtxvideo/ngx_guard.hpp"
#include "enh/pipeline/stage/upscaler.hpp"

#include <cstdint>
#include <filesystem>
#include <memory>

namespace enh::pipeline::rtxvideo {

/// Lignes de NGX renvoyees au journal du pipeline : Off en production. Quel que soit ce choix,
/// NGX n'ecrit aucun fichier (DisableOtherLoggingSinks) et ses lignes de niveau ON sont lues EN
/// MEMOIRE par NgxLogObserver (DLL de fonction utilisee), puis oubliees.
enum class NgxLogLevel : std::uint8_t { Off, On, Verbose };

struct NgxRuntimeOptions {
    std::filesystem::path data_dir; // cree s'il n'existe pas ; %LOCALAPPDATA%\Enhance\ngx en appli
    void* cu_context = nullptr;     // CUcontext (contexte primaire du pipeline)
    NgxLogLevel log_level = NgxLogLevel::Off;
};

struct VsrCapabilities {
    bool available = false;
    bool needs_updated_driver = false;
    std::uint32_t min_driver_major = 0;
    std::uint32_t min_driver_minor = 0;
    std::uint64_t scratch_bytes = 0;
};

class NgxRuntime {
public:
    [[nodiscard]] static Result<NgxRuntime> create(const BridgeApi& api,
                                                   const NgxRuntimeOptions& options);
    NgxRuntime(NgxRuntime&& other) noexcept;
    NgxRuntime& operator=(NgxRuntime&& other) noexcept;
    NgxRuntime(const NgxRuntime&) = delete;
    NgxRuntime& operator=(const NgxRuntime&) = delete;
    ~NgxRuntime();

    [[nodiscard]] Result<VsrCapabilities> queryVsr() const;

    [[nodiscard]] const BridgeApi& api() const noexcept { return api_; }
    [[nodiscard]] rvb_runtime* handle() const noexcept { return runtime_; }

    /// DLL de fonction VSR annoncee par NGX dans son journal (vide tant qu'aucun effet n'est cree).
    [[nodiscard]] NgxSnippetInfo vsrSnippet() const;
    /// La creation de processus enfants (mise a jour NGX) etait bloquee pendant l'initialisation.
    [[nodiscard]] bool updaterBlocked() const noexcept { return updater_blocked_; }

    struct LogContext; // journal NGX : observateur + renvoi eventuel vers le journal du pipeline

private:
    NgxRuntime(const BridgeApi& api, rvb_runtime* runtime, std::unique_ptr<LogContext> log,
               bool updater_blocked) noexcept;
    void reset() noexcept;

    BridgeApi api_;
    std::unique_ptr<LogContext> log_; // adresse stable, survit a l'arret de NGX (voir reset())
    rvb_runtime* runtime_ = nullptr;
    bool updater_blocked_ = false;
};

class VsrEffect {
public:
    /// `cu_stream` : flux sur lequel toute la super-resolution est mise en file (CUstream).
    [[nodiscard]] static Result<VsrEffect> create(const NgxRuntime& runtime, void* cu_context,
                                                  void* cu_stream);
    VsrEffect(VsrEffect&& other) noexcept;
    VsrEffect& operator=(VsrEffect&& other) noexcept;
    VsrEffect(const VsrEffect&) = delete;
    VsrEffect& operator=(const VsrEffect&) = delete;
    ~VsrEffect();

    /// Boucle chaude : aucune allocation en cas de succes, aucune synchronisation.
    [[nodiscard]] Result<void> evaluate(const UpscaleRequest& request) noexcept;

private:
    VsrEffect(const BridgeApi& api, rvb_effect* effect) noexcept : api_(api), effect_(effect) {}
    void reset() noexcept;

    BridgeApi api_;
    rvb_effect* effect_ = nullptr;
};

} // namespace enh::pipeline::rtxvideo
