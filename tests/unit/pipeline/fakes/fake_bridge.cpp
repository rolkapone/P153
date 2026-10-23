#include "fakes/fake_bridge.hpp"

// Les types opaques de l'API C sont definis ici pour le faux (le vrai pont a les siens, dans une
// autre DLL).
struct rvb_runtime {
    int id = 0;
};
struct rvb_effect {
    int id = 0;
};

namespace enh::pipeline::fakes {

namespace {

FakeBridgeState g_state;
rvb_runtime g_runtime{1};
rvb_effect g_effect{2};

uint32_t RVB_CALL fakeAbiVersion() {
    g_state.calls.emplace_back("abi_version");
    return RVB_ABI_VERSION;
}

rvb_result RVB_CALL fakeRuntimeCreate(const rvb_init_params* p, rvb_runtime** out) {
    g_state.calls.emplace_back("runtime_create");
    if (p == nullptr || out == nullptr) {
        return RVB_E_INVALID_ARG;
    }
    g_state.init_struct_size = p->struct_size;
    g_state.init_log_level = p->log_level;
    g_state.init_has_log_fn = p->log_fn != nullptr;
    g_state.init_data_path = p->data_path != nullptr ? p->data_path : L"";
    g_state.init_context = p->cu_context;
    g_state.log_fn = p->log_fn;
    g_state.log_user = p->log_user;
    if (g_state.runtime_create_result != RVB_OK) {
        return g_state.runtime_create_result;
    }
    if (g_state.live_runtimes > 0) {
        return RVB_E_ALREADY_INITIALIZED;
    }
    ++g_state.live_runtimes;
    *out = &g_runtime;
    return RVB_OK;
}

rvb_result RVB_CALL fakeRuntimeDestroy(rvb_runtime* rt) {
    g_state.calls.emplace_back("runtime_destroy");
    if (rt != &g_runtime) {
        return RVB_E_INVALID_ARG;
    }
    if (g_state.live_effects > 0) {
        return RVB_E_BUSY;
    }
    --g_state.live_runtimes;
    return g_state.runtime_destroy_result;
}

rvb_result RVB_CALL fakeVsrQuery(rvb_runtime* rt, rvb_vsr_caps* out) {
    g_state.calls.emplace_back("vsr_query");
    if (rt != &g_runtime || out == nullptr) {
        return RVB_E_INVALID_ARG;
    }
    if (out->struct_size < sizeof(rvb_vsr_caps)) {
        return RVB_E_BAD_STRUCT_SIZE;
    }
    if (g_state.query_result != RVB_OK) {
        return g_state.query_result;
    }
    *out = g_state.caps;
    return RVB_OK;
}

rvb_result RVB_CALL fakeVsrCreate(rvb_runtime* rt, const rvb_vsr_create_params* p,
                                  rvb_effect** out) {
    g_state.calls.emplace_back("vsr_create");
    if (rt != &g_runtime || p == nullptr || out == nullptr) {
        return RVB_E_INVALID_ARG;
    }
    g_state.create_stream = p->cu_stream;
    if (!g_state.log_line_on_vsr_create.empty() && g_state.log_fn != nullptr) {
        g_state.log_fn(g_state.log_user, RVB_LOG_ON, g_state.log_line_on_vsr_create.c_str());
    }
    if (g_state.vsr_create_result != RVB_OK) {
        return g_state.vsr_create_result;
    }
    ++g_state.live_effects;
    *out = &g_effect;
    return RVB_OK;
}

rvb_result RVB_CALL fakeVsrEvaluate(rvb_effect* fx, const rvb_vsr_eval_params* p) {
    // Pas d'enregistrement de l'appel : la boucle chaude n'alloue pas, meme dans le faux.
    if (fx != &g_effect || p == nullptr) {
        return RVB_E_INVALID_ARG;
    }
    g_state.last_eval = *p;
    ++g_state.evaluations;
    return g_state.evaluate_result;
}

rvb_result RVB_CALL fakeEffectDestroy(rvb_effect* fx) {
    g_state.calls.emplace_back("effect_destroy");
    if (fx != &g_effect) {
        return RVB_E_INVALID_ARG;
    }
    --g_state.live_effects;
    return g_state.effect_destroy_result;
}

} // namespace

FakeBridgeState& fakeBridge() {
    return g_state;
}

void resetFakeBridge() {
    g_state = FakeBridgeState{};
}

rtxvideo::BridgeApi fakeBridgeApi() {
    rtxvideo::BridgeApi api;
    api.abi_version = fakeAbiVersion;
    api.runtime_create = fakeRuntimeCreate;
    api.runtime_destroy = fakeRuntimeDestroy;
    api.vsr_query = fakeVsrQuery;
    api.vsr_create = fakeVsrCreate;
    api.vsr_evaluate = fakeVsrEvaluate;
    api.effect_destroy = fakeEffectDestroy;
    return api;
}

} // namespace enh::pipeline::fakes
