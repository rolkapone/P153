// enh_rtxvideo_bridge.dll : API C (enh/rtxvideo_bridge.h) au-dessus de NGX / RTX Video SDK 1.1.0.
// Lie nvsdk_ngx_s.lib (CRT statique /MT) et l'API pilote CUDA (nvcuda.dll), JAMAIS cudart : les
// trois fonctions du runtime CUDA attendues par NGX sont dans cudart_shim.cpp.
//
// Neutralisation de la mise a jour de NGX (SEC-01, docs/gpu/ENH-040.md § 4) :
// - aucune fonction de mise a jour de fonction NGX n'est appelee, ni aucune variante d'Init par
//   identifiant de projet (controle statique : scripts/check-no-network.ps1 C1 et
//   tests/unit/pipeline/rtxvideo/ngx_static_policy_test.cpp) ;
// - identifiant d'application 0, aucune description de projet (« Required for over the air
//   updates », nvsdk_ngx_defs.h, NVSDK_NGX_Application_Identifier) ;
// - PathListInfo vide : NGX ne cherche la DLL de la fonction que dans le dossier de l'application ;
// - journal : niveau choisi par l'application (coupe en release) et DisableOtherLoggingSinks = true
//   (NGX n'ecrit aucun fichier journal ; tout passe par notre rappel).
//
// API NGX utilisees (en-tetes du SDK 1.1.0, third_party/nvidia/rtx-video-sdk/include) :
//   NVSDK_NGX_CUDA_Init (surcharge avec NVSDK_NGX_FeatureCommonInfo)     nvsdk_ngx.h l. 160
//   NVSDK_NGX_CUDA_GetCapabilityParameters / _DestroyParameters          nvsdk_ngx.h l. 317 / 339
//   NVSDK_NGX_CUDA_GetScratchBufferSize                                  nvsdk_ngx.h l. 365
//   NVSDK_NGX_CUDA_ReleaseFeature / NVSDK_NGX_CUDA_Shutdown              nvsdk_ngx.h l. 419 / 225
//   NGX_CUDA_CREATE_VSR / NGX_CUDA_EVALUATE_VSR                          nvsdk_ngx_helpers_vsr.h
//   VSR.Available, VSR.NeedsUpdatedDriver, VSR.MinDriverVersion*         nvsdk_ngx_defs_vsr.h

#include "enh/rtxvideo_bridge.h"

// nvsdk_ngx.h declare aussi les points d'entree D3D11 / D3D12 : leurs types doivent etre connus.
// clang-format off
#include <windows.h>
#include <d3d11.h>
#include <d3d12.h>
// clang-format on

#include <cuda.h>
#include <nvsdk_ngx.h>
#include <nvsdk_ngx_defs.h>
#include <nvsdk_ngx_helpers_vsr.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>

struct rvb_runtime {
    NVSDK_NGX_Parameter* params = nullptr; // parametres de capacite (partages par les effets)
    CUcontext context = nullptr;
    std::mutex params_mutex; // NGX_CUDA_EVALUATE_VSR ecrit dans `params` : un appel a la fois
    std::atomic<int> live_effects{0};
};

struct rvb_effect {
    rvb_runtime* runtime = nullptr;
    NVSDK_NGX_Handle* handle = nullptr;
    CUcontext context = nullptr;
};

namespace {

constexpr unsigned int kMaxQuality = NVSDK_NGX_VSR_Quality_Ultra;

// NGX ne transmet pas de pointeur utilisateur a son rappel : etat global (un runtime par
// processus).
std::atomic<rvb_runtime*> g_runtime{nullptr};
std::atomic<rvb_log_fn> g_log_fn{nullptr};
std::atomic<void*> g_log_user{nullptr};
std::atomic<std::uint32_t> g_log_level{RVB_LOG_OFF};

void NVSDK_CONV ngxLog(const char* message, NVSDK_NGX_Logging_Level level,
                       NVSDK_NGX_Feature /*feature*/) {
    if (g_log_level.load(std::memory_order_relaxed) == RVB_LOG_OFF || message == nullptr) {
        return; // release : rien n'est ecrit, nulle part
    }
    rvb_log_fn fn = g_log_fn.load(std::memory_order_acquire);
    if (fn != nullptr) {
        fn(g_log_user.load(std::memory_order_acquire), static_cast<std::uint32_t>(level), message);
    }
}

/// Rend le contexte CUDA courant le temps d'un appel NGX (quel que soit le thread appelant).
class ScopedContext {
public:
    explicit ScopedContext(CUcontext ctx) noexcept
        : ok_(ctx != nullptr && cuCtxPushCurrent(ctx) == CUDA_SUCCESS) {}
    ScopedContext(const ScopedContext&) = delete;
    ScopedContext& operator=(const ScopedContext&) = delete;
    ScopedContext(ScopedContext&&) = delete;
    ScopedContext& operator=(ScopedContext&&) = delete;
    ~ScopedContext() {
        if (ok_) {
            CUcontext popped = nullptr;
            (void)cuCtxPopCurrent(&popped);
        }
    }
    [[nodiscard]] bool ok() const noexcept { return ok_; }

private:
    bool ok_;
};

/// Aucune exception ne traverse la frontiere C : tout est converti en RVB_E_INTERNAL.
template <typename F> rvb_result guarded(F&& body) noexcept {
    try {
        return body();
    } catch (...) {
        return RVB_E_INTERNAL;
    }
}

template <typename T> bool sizeOk(const T* p) noexcept {
    return p->struct_size >= sizeof(T);
}

} // namespace

extern "C" {

uint32_t RVB_CALL rvb_abi_version(void) {
    return RVB_ABI_VERSION;
}

rvb_result RVB_CALL rvb_runtime_create(const rvb_init_params* params, rvb_runtime** out) {
    return guarded([&]() -> rvb_result {
        if (params == nullptr || out == nullptr || params->data_path == nullptr ||
            params->cu_context == nullptr || params->log_level > RVB_LOG_VERBOSE) {
            return RVB_E_INVALID_ARG;
        }
        if (!sizeOk(params)) {
            return RVB_E_BAD_STRUCT_SIZE;
        }
        *out = nullptr;
        auto rt = std::make_unique<rvb_runtime>();
        rvb_runtime* expected = nullptr;
        if (!g_runtime.compare_exchange_strong(expected, rt.get())) {
            return RVB_E_ALREADY_INITIALIZED;
        }
        rt->context = static_cast<CUcontext>(params->cu_context);
        g_log_user.store(params->log_user, std::memory_order_release);
        g_log_fn.store(params->log_fn, std::memory_order_release);
        g_log_level.store(params->log_level, std::memory_order_relaxed);

        const ScopedContext scope(rt->context);
        if (!scope.ok()) {
            g_runtime.store(nullptr);
            return RVB_E_CUDA;
        }
        NVSDK_NGX_FeatureCommonInfo info{}; // PathListInfo vide : dossier de l'application seul
        info.LoggingInfo.LoggingCallback = ngxLog;
        info.LoggingInfo.MinimumLoggingLevel =
            params->log_level == RVB_LOG_VERBOSE ? NVSDK_NGX_LOGGING_LEVEL_VERBOSE
            : params->log_level == RVB_LOG_ON    ? NVSDK_NGX_LOGGING_LEVEL_ON
                                                 : NVSDK_NGX_LOGGING_LEVEL_OFF;
        info.LoggingInfo.DisableOtherLoggingSinks = true;

        constexpr unsigned long long kApplicationId = 0; // SEC-01 : jamais d'identifiant
        NVSDK_NGX_Result r = NVSDK_NGX_CUDA_Init(kApplicationId, params->data_path, &info);
        if (NVSDK_NGX_FAILED(r)) {
            g_runtime.store(nullptr);
            return static_cast<rvb_result>(r);
        }
        r = NVSDK_NGX_CUDA_GetCapabilityParameters(&rt->params);
        if (NVSDK_NGX_FAILED(r) || rt->params == nullptr) {
            (void)NVSDK_NGX_CUDA_Shutdown();
            g_runtime.store(nullptr);
            return NVSDK_NGX_FAILED(r) ? static_cast<rvb_result>(r) : RVB_E_INTERNAL;
        }
        *out = rt.release(); // detruit par rvb_runtime_destroy
        return RVB_OK;
    });
}

rvb_result RVB_CALL rvb_runtime_destroy(rvb_runtime* runtime) {
    return guarded([&]() -> rvb_result {
        if (runtime == nullptr || g_runtime.load() != runtime) {
            return RVB_E_INVALID_ARG;
        }
        if (runtime->live_effects.load() != 0) {
            return RVB_E_BUSY; // l'appelant doit d'abord detruire ses effets
        }
        const std::unique_ptr<rvb_runtime> owned(runtime);
        NVSDK_NGX_Result r = NVSDK_NGX_Result_Success;
        {
            const ScopedContext scope(owned->context);
            if (owned->params != nullptr) {
                (void)NVSDK_NGX_CUDA_DestroyParameters(owned->params);
            }
            r = NVSDK_NGX_CUDA_Shutdown();
        }
        g_log_fn.store(nullptr, std::memory_order_release);
        g_log_user.store(nullptr, std::memory_order_release);
        g_log_level.store(RVB_LOG_OFF);
        g_runtime.store(nullptr);
        return NVSDK_NGX_FAILED(r) ? static_cast<rvb_result>(r) : RVB_OK;
    });
}

rvb_result RVB_CALL rvb_vsr_query(rvb_runtime* runtime, rvb_vsr_caps* out) {
    return guarded([&]() -> rvb_result {
        if (runtime == nullptr || out == nullptr) {
            return RVB_E_INVALID_ARG;
        }
        if (!sizeOk(out)) {
            return RVB_E_BAD_STRUCT_SIZE;
        }
        const std::scoped_lock lock(runtime->params_mutex);
        int available = 0;
        int needs_driver = 0;
        unsigned int major = 0;
        unsigned int minor = 0;
        // Get() renvoie un echec si la cle est absente : la valeur reste alors a 0.
        (void)runtime->params->Get(NVSDK_NGX_Parameter_VSR_Available, &available);
        (void)runtime->params->Get(NVSDK_NGX_Parameter_VSR_NeedsUpdatedDriver, &needs_driver);
        (void)runtime->params->Get(NVSDK_NGX_Parameter_VSR_MinDriverVersionMajor, &major);
        (void)runtime->params->Get(NVSDK_NGX_Parameter_VSR_MinDriverVersionMinor, &minor);
        std::size_t scratch = 0;
        const NVSDK_NGX_Result r =
            NVSDK_NGX_CUDA_GetScratchBufferSize(NVSDK_NGX_Feature_VSR, runtime->params, &scratch);
        out->available = available != 0 ? 1U : 0U;
        out->needs_updated_driver = needs_driver != 0 ? 1U : 0U;
        out->min_driver_major = major;
        out->min_driver_minor = minor;
        out->reserved = 0;
        out->scratch_bytes = scratch;
        // VSR indisponible : GetScratchBufferSize peut echouer ; la reponse reste exploitable.
        return NVSDK_NGX_FAILED(r) && available != 0 ? static_cast<rvb_result>(r) : RVB_OK;
    });
}

rvb_result RVB_CALL rvb_vsr_create(rvb_runtime* runtime, const rvb_vsr_create_params* params,
                                   rvb_effect** out) {
    return guarded([&]() -> rvb_result {
        if (runtime == nullptr || params == nullptr || out == nullptr ||
            params->cu_context == nullptr) {
            return RVB_E_INVALID_ARG;
        }
        if (!sizeOk(params)) {
            return RVB_E_BAD_STRUCT_SIZE;
        }
        *out = nullptr;
        auto fx = std::make_unique<rvb_effect>();
        fx->runtime = runtime;
        fx->context = static_cast<CUcontext>(params->cu_context);
        const ScopedContext scope(fx->context);
        if (!scope.ok()) {
            return RVB_E_CUDA;
        }
        NVSDK_NGX_CUDA_VSR_Create_Params p{};
        p.InCUContext = params->cu_context;
        p.InCUStream = params->cu_stream;
        const std::scoped_lock lock(runtime->params_mutex);
        const NVSDK_NGX_Result r = NGX_CUDA_CREATE_VSR(&fx->handle, runtime->params, &p);
        if (NVSDK_NGX_FAILED(r)) {
            return static_cast<rvb_result>(r);
        }
        if (fx->handle == nullptr) {
            return RVB_E_INTERNAL;
        }
        runtime->live_effects.fetch_add(1);
        *out = fx.release(); // detruit par rvb_effect_destroy
        return RVB_OK;
    });
}

rvb_result RVB_CALL rvb_vsr_evaluate(rvb_effect* effect, const rvb_vsr_eval_params* params) {
    return guarded([&]() -> rvb_result {
        if (effect == nullptr || params == nullptr) {
            return RVB_E_INVALID_ARG;
        }
        if (!sizeOk(params)) {
            return RVB_E_BAD_STRUCT_SIZE;
        }
        if (params->quality > kMaxQuality || params->input_texture == 0 ||
            params->output_surface == 0 || params->input_rect.width == 0 ||
            params->input_rect.height == 0 || params->output_rect.width == 0 ||
            params->output_rect.height == 0) {
            return RVB_E_INVALID_ARG;
        }
        CUtexObject input = params->input_texture;
        CUsurfObject output = params->output_surface;
        NVSDK_NGX_CUDA_VSR_Eval_Params p{};
        p.pInput = &input; // NGX attend l'adresse de l'objet texture / surface
        p.pOutput = &output;
        p.InputSubrectBase = {.X = params->input_rect.x, .Y = params->input_rect.y};
        p.InputSubrectSize = {.Width = params->input_rect.width,
                              .Height = params->input_rect.height};
        p.OutputSubrectBase = {.X = params->output_rect.x, .Y = params->output_rect.y};
        p.OutputSubrectSize = {.Width = params->output_rect.width,
                               .Height = params->output_rect.height};
        p.QualityLevel = static_cast<NVSDK_NGX_VSR_QualityLevel>(params->quality);
        const ScopedContext scope(effect->context);
        if (!scope.ok()) {
            return RVB_E_CUDA;
        }
        const std::scoped_lock lock(effect->runtime->params_mutex);
        const NVSDK_NGX_Result r =
            NGX_CUDA_EVALUATE_VSR(effect->handle, effect->runtime->params, &p);
        return NVSDK_NGX_FAILED(r) ? static_cast<rvb_result>(r) : RVB_OK;
    });
}

rvb_result RVB_CALL rvb_effect_destroy(rvb_effect* effect) {
    return guarded([&]() -> rvb_result {
        if (effect == nullptr) {
            return RVB_E_INVALID_ARG;
        }
        const std::unique_ptr<rvb_effect> owned(effect);
        NVSDK_NGX_Result r = NVSDK_NGX_Result_Success;
        {
            const ScopedContext scope(owned->context);
            r = NVSDK_NGX_CUDA_ReleaseFeature(owned->handle);
        }
        owned->runtime->live_effects.fetch_sub(1);
        return NVSDK_NGX_FAILED(r) ? static_cast<rvb_result>(r) : RVB_OK;
    });
}

} // extern "C"
