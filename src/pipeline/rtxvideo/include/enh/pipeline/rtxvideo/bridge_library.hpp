#pragma once

// Chargement explicite de enh_rtxvideo_bridge.dll (ADR-048 amende, docs/gpu/ENH-040.md § 2) :
// LoadLibraryExW sur un chemin ABSOLU avec LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | SYSTEM32, puis
// GetProcAddress et controle de la version d'ABI. L'application demarre sans le pont (ENH-031) :
// son absence est une erreur ComponentMissing{RtxVideoSdk}, jamais un plantage.

#include "enh/pipeline/error.hpp"
#include "enh/pipeline/rtxvideo/bridge_api.hpp"

#include <filesystem>
#include <memory>

namespace enh::pipeline::rtxvideo {

inline constexpr const wchar_t* kBridgeDllName = L"enh_rtxvideo_bridge.dll";
inline constexpr const wchar_t* kVsrFeatureDllName = L"nvngx_vsr.dll";

/// Dossier de l'executable courant (GetModuleFileNameW(nullptr)).
[[nodiscard]] Result<std::filesystem::path> applicationDirectory();

/// RAII : la DLL est dechargee (FreeLibrary) a la destruction. Deplacable, non copiable.
class BridgeLibrary {
public:
    /// `dll_path` doit etre absolu. Erreurs : InvalidParams (chemin relatif),
    /// ComponentMissing{RtxVideoSdk} (DLL absente, export manquant, ABI differente).
    [[nodiscard]] static Result<BridgeLibrary> load(const std::filesystem::path& dll_path);

    [[nodiscard]] const BridgeApi& api() const noexcept { return api_; }

private:
    struct ModuleDeleter {
        void operator()(void* module) const noexcept;
    };
    BridgeLibrary(std::unique_ptr<void, ModuleDeleter> module, const BridgeApi& api) noexcept
        : module_(std::move(module)), api_(api) {}

    std::unique_ptr<void, ModuleDeleter> module_;
    BridgeApi api_;
};

} // namespace enh::pipeline::rtxvideo
