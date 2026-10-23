#pragma once

// Faux pont RTX Video pour la CI (docs/gpu/ENH-040.md § 6) : table BridgeApi dont les fonctions
// enregistrent leurs appels et imitent les regles du vrai pont (un runtime par processus,
// arret refuse tant qu'un effet vit). Etat global : reinitialiser par resetFakeBridge().

#include "enh/pipeline/rtxvideo/bridge_api.hpp"

#include <string>
#include <vector>

namespace enh::pipeline::fakes {

struct FakeBridgeState {
    std::vector<std::string> calls;
    rvb_result runtime_create_result = RVB_OK;
    rvb_result runtime_destroy_result = RVB_OK;
    rvb_result query_result = RVB_OK;
    rvb_result vsr_create_result = RVB_OK;
    rvb_result evaluate_result = RVB_OK;
    rvb_result effect_destroy_result = RVB_OK;
    rvb_vsr_caps caps{sizeof(rvb_vsr_caps), 1, 0, 0, 0, 0, 0};
    int live_runtimes = 0;
    int live_effects = 0;
    int evaluations = 0;
    std::uint32_t init_struct_size = 0;
    std::uint32_t init_log_level = 0;
    bool init_has_log_fn = false;
    std::wstring init_data_path;
    void* init_context = nullptr;
    void* create_stream = nullptr;
    rvb_vsr_eval_params last_eval{};
    std::string log_line_on_vsr_create; // ligne de journal NGX emise pendant rvb_vsr_create
    rvb_log_fn log_fn = nullptr;
    void* log_user = nullptr;
};

FakeBridgeState& fakeBridge();
void resetFakeBridge();
rtxvideo::BridgeApi fakeBridgeApi();

} // namespace enh::pipeline::fakes
