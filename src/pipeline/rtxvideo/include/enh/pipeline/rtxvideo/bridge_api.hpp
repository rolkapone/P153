#pragma once

// Table des fonctions du pont RTX Video (enh/rtxvideo_bridge.h) et traduction de ses codes retour
// en PipelineError (ADR-047). Aucune dependance CUDA : compile et teste en CI avec une fausse
// table.

#include "enh/pipeline/error.hpp"
#include "enh/rtxvideo_bridge.h"

#include <source_location>
#include <string_view>

namespace enh::pipeline::rtxvideo {

/// Pointeurs vers l'API C du pont (GetProcAddress, ou faux en test). Copiable : 7 pointeurs.
/// Les fonctions doivent rester valides tant qu'un objet qui les utilise existe (la DLL reste
/// chargee : voir l'ordre des membres de RtxVideoUpscaler).
struct BridgeApi {
    rvb_abi_version_fn abi_version = nullptr;
    rvb_runtime_create_fn runtime_create = nullptr;
    rvb_runtime_destroy_fn runtime_destroy = nullptr;
    rvb_vsr_query_fn vsr_query = nullptr;
    rvb_vsr_create_fn vsr_create = nullptr;
    rvb_vsr_evaluate_fn vsr_evaluate = nullptr;
    rvb_effect_destroy_fn effect_destroy = nullptr;

    [[nodiscard]] bool complete() const noexcept {
        return abi_version != nullptr && runtime_create != nullptr && runtime_destroy != nullptr &&
               vsr_query != nullptr && vsr_create != nullptr && vsr_evaluate != nullptr &&
               effect_destroy != nullptr;
    }
};

/// Meme regle que rvb_succeeded() (en-tete C), utilisable en constexpr.
[[nodiscard]] constexpr bool succeeded(rvb_result r) noexcept {
    return (r & RVB_NGX_FAIL_MASK) != RVB_NGX_FAIL_BASE && (r & 0xF0000000U) != RVB_E_BASE;
}

/// Nom stable d'un code (journal, tests) : "FAIL_OutOfGPUMemory", "RVB_E_BUSY"...
[[nodiscard]] std::string_view resultName(rvb_result r) noexcept;

/// Code du pont ou de NGX -> PipelineError :
/// - fonction absente / non prise en charge / refusee -> ComponentMissing{RtxVideoSdk} ;
/// - pilote trop ancien (FAIL_OutOfDate) -> ComponentMissing{Driver} ;
/// - memoire GPU -> OutOfVram ; parametre ou format refuse -> InvalidParams ;
/// - dossier de donnees non inscriptible -> WriteDenied ; runtime deja present -> Busy ;
/// - le reste -> Internal. native_code = code d'origine.
[[nodiscard]] PipelineError
bridgeFailure(rvb_result r, std::string_view what,
              std::source_location where = std::source_location::current());

[[nodiscard]] inline Result<void>
checkBridge(rvb_result r, std::string_view what,
            std::source_location where = std::source_location::current()) {
    if (succeeded(r)) {
        return {};
    }
    return enh::Unexpected<PipelineError>(bridgeFailure(r, what, where));
}

} // namespace enh::pipeline::rtxvideo
