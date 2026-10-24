// Fausse DLL pont (tests CI de BridgeLibrary : chargement reel par LoadLibraryExW + GetProcAddress,
// controle d'ABI). ENH_FAKE_ABI fixe la version renvoyee ; ENH_FAKE_PARTIAL omet un export.
// Aucune fonction n'est appelee au-dela de rvb_abi_version. Exports : fake_bridge*.def.

#include "enh/rtxvideo_bridge.h"

#ifndef ENH_FAKE_ABI
#define ENH_FAKE_ABI RVB_ABI_VERSION
#endif

// NOLINTBEGIN(readability-identifier-naming) : noms imposes par l'API C du pont
extern "C" {

uint32_t RVB_CALL rvb_abi_version(void) {
    return ENH_FAKE_ABI;
}
rvb_result RVB_CALL rvb_runtime_create(const rvb_init_params* /*p*/, rvb_runtime** /*out*/) {
    return RVB_E_INTERNAL;
}
rvb_result RVB_CALL rvb_runtime_destroy(rvb_runtime* /*rt*/) {
    return RVB_E_INTERNAL;
}
rvb_result RVB_CALL rvb_vsr_query(rvb_runtime* /*rt*/, rvb_vsr_caps* /*out*/) {
    return RVB_E_INTERNAL;
}
rvb_result RVB_CALL rvb_vsr_create(rvb_runtime* /*rt*/, const rvb_vsr_create_params* /*p*/,
                                   rvb_effect** /*out*/) {
    return RVB_E_INTERNAL;
}
rvb_result RVB_CALL rvb_vsr_evaluate(rvb_effect* /*fx*/, const rvb_vsr_eval_params* /*p*/) {
    return RVB_E_INTERNAL;
}
#ifndef ENH_FAKE_PARTIAL
rvb_result RVB_CALL rvb_effect_destroy(rvb_effect* /*fx*/) {
    return RVB_E_INTERNAL;
}
#endif

} // extern "C"
// NOLINTEND(readability-identifier-naming)
