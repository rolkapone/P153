#include "enh/pipeline/rtxvideo/ngx_runtime.hpp"

#include "enh/pipeline/core/log.hpp"

#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace enh::pipeline::rtxvideo {

struct NgxRuntime::LogContext {
    NgxLogObserver observer;
    bool forward_to_journal = false;
};

namespace {

// Appele par NGX depuis n'importe quel thread : observation en memoire, renvoi au journal du
// pipeline seulement si demande. Jamais d'exception vers le pont.
void RVB_CALL onNgxLog(void* user, std::uint32_t /*level*/, const char* message) noexcept {
    if (user == nullptr || message == nullptr) {
        return;
    }
    try {
        auto* ctx = static_cast<NgxRuntime::LogContext*>(user);
        std::string_view line(message);
        while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) {
            line.remove_suffix(1);
        }
        ctx->observer.onLine(line);
        if (ctx->forward_to_journal) {
            logLine(LogLevel::Debug, "NgxLog", line);
        }
    } catch (...) { // NOLINT(bugprone-empty-catch) : une ligne de journal perdue, rien de plus
    }
}

/// Niveau demande a NGX : au moins ON, pour que NgxLogObserver voie la DLL de fonction utilisee.
std::uint32_t toBridgeLevel(NgxLogLevel level) noexcept {
    return level == NgxLogLevel::Verbose ? RVB_LOG_VERBOSE : RVB_LOG_ON;
}

/// Champs a zero puis struct_size renseigne (versionnage de l'ABI par la taille).
template <typename T> T sized() noexcept {
    T v{};
    v.struct_size = sizeof(T);
    return v;
}

} // namespace

// ---------------------------------------------------------------------------------------------
// NgxRuntime
// ---------------------------------------------------------------------------------------------

Result<NgxRuntime> NgxRuntime::create(const BridgeApi& api, const NgxRuntimeOptions& options) {
    if (!api.complete()) {
        return fail(PipelineErrorCode::Internal, "NgxRuntime: incomplete bridge API");
    }
    if (options.cu_context == nullptr || options.data_dir.empty()) {
        return fail(PipelineErrorCode::InvalidParams,
                    "NgxRuntime: CUDA context and data dir required");
    }
    std::error_code ec;
    std::filesystem::create_directories(options.data_dir, ec);
    if (ec) {
        return fail(PipelineErrorCode::WriteDenied, "NgxRuntime: cannot create NGX data dir",
                    ec.value());
    }
    const std::wstring data_path = options.data_dir.wstring();
    auto log = std::make_unique<LogContext>();
    log->forward_to_journal = options.log_level != NgxLogLevel::Off;
    auto params = sized<rvb_init_params>();
    params.log_level = toBridgeLevel(options.log_level);
    params.data_path = data_path.c_str();
    params.cu_context = options.cu_context;
    params.log_fn = onNgxLog;
    params.log_user = log.get();

    rvb_runtime* runtime = nullptr;
    rvb_result created = RVB_E_INTERNAL;
    bool blocked = false;
    {
        const ChildProcessBlocker blocker; // pas de nvngx_update.exe pendant l'initialisation
        blocked = blocker.active();
        created = api.runtime_create(&params, &runtime);
    }
    if (auto r = checkBridge(created, "rvb_runtime_create"); !r) {
        return enh::Unexpected<PipelineError>(std::move(r.error()));
    }
    if (runtime == nullptr) {
        return fail(PipelineErrorCode::Internal, "rvb_runtime_create returned no runtime");
    }
    return NgxRuntime(api, runtime, std::move(log), blocked);
}

NgxRuntime::NgxRuntime(const BridgeApi& api, rvb_runtime* runtime, std::unique_ptr<LogContext> log,
                       bool updater_blocked) noexcept
    : api_(api), log_(std::move(log)), runtime_(runtime), updater_blocked_(updater_blocked) {}

NgxRuntime::NgxRuntime(NgxRuntime&& other) noexcept
    : api_(other.api_), log_(std::move(other.log_)),
      runtime_(std::exchange(other.runtime_, nullptr)), updater_blocked_(other.updater_blocked_) {}

NgxRuntime& NgxRuntime::operator=(NgxRuntime&& other) noexcept {
    if (this != &other) {
        reset();
        api_ = other.api_;
        log_ = std::move(other.log_);
        runtime_ = std::exchange(other.runtime_, nullptr);
        updater_blocked_ = other.updater_blocked_;
    }
    return *this;
}

NgxSnippetInfo NgxRuntime::vsrSnippet() const {
    return log_ != nullptr ? log_->observer.vsrSnippet() : NgxSnippetInfo{};
}

NgxRuntime::~NgxRuntime() {
    reset();
}

void NgxRuntime::reset() noexcept {
    if (runtime_ != nullptr) {
        // Arret de NGX AVANT de liberer le contexte de journal (NGX peut encore journaliser).
        const rvb_result r = api_.runtime_destroy(std::exchange(runtime_, nullptr));
        if (!succeeded(r)) {
            logLine(LogLevel::Warn, "NgxShutdownFailed", resultName(r));
        }
    }
    log_.reset();
}

Result<VsrCapabilities> NgxRuntime::queryVsr() const {
    if (runtime_ == nullptr) {
        return fail(PipelineErrorCode::Internal, "NgxRuntime: moved-from runtime");
    }
    auto caps = sized<rvb_vsr_caps>();
    if (auto r = checkBridge(api_.vsr_query(runtime_, &caps), "rvb_vsr_query"); !r) {
        return enh::Unexpected<PipelineError>(std::move(r.error()));
    }
    VsrCapabilities out;
    out.available = caps.available != 0;
    out.needs_updated_driver = caps.needs_updated_driver != 0;
    out.min_driver_major = caps.min_driver_major;
    out.min_driver_minor = caps.min_driver_minor;
    out.scratch_bytes = caps.scratch_bytes;
    return out;
}

// ---------------------------------------------------------------------------------------------
// VsrEffect
// ---------------------------------------------------------------------------------------------

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) : poignees CUDA opaques de l API C
Result<VsrEffect> VsrEffect::create(const NgxRuntime& runtime, void* cu_context, void* cu_stream) {
    if (runtime.handle() == nullptr || cu_context == nullptr) {
        return fail(PipelineErrorCode::InvalidParams,
                    "VsrEffect: runtime and CUDA context required");
    }
    auto params = sized<rvb_vsr_create_params>();
    params.cu_context = cu_context;
    params.cu_stream = cu_stream;
    rvb_effect* effect = nullptr;
    const BridgeApi& api = runtime.api();
    rvb_result created = RVB_E_INTERNAL;
    {
        const ChildProcessBlocker blocker; // NGX choisit et charge ici la DLL de fonction
        created = api.vsr_create(runtime.handle(), &params, &effect);
    }
    if (auto r = checkBridge(created, "rvb_vsr_create"); !r) {
        return enh::Unexpected<PipelineError>(std::move(r.error()));
    }
    if (effect == nullptr) {
        return fail(PipelineErrorCode::Internal, "rvb_vsr_create returned no effect");
    }
    return VsrEffect(api, effect);
}

VsrEffect::VsrEffect(VsrEffect&& other) noexcept
    : api_(other.api_), effect_(std::exchange(other.effect_, nullptr)) {}

VsrEffect& VsrEffect::operator=(VsrEffect&& other) noexcept {
    if (this != &other) {
        reset();
        api_ = other.api_;
        effect_ = std::exchange(other.effect_, nullptr);
    }
    return *this;
}

VsrEffect::~VsrEffect() {
    reset();
}

void VsrEffect::reset() noexcept {
    if (effect_ == nullptr) {
        return;
    }
    const rvb_result r = api_.effect_destroy(std::exchange(effect_, nullptr));
    if (!succeeded(r)) {
        logLine(LogLevel::Warn, "NgxReleaseFeatureFailed", resultName(r));
    }
}

// NOLINTNEXTLINE(bugprone-exception-escape) : seule l'erreur (rare) alloue ; bad_alloc = arret
Result<void> VsrEffect::evaluate(const UpscaleRequest& request) noexcept {
    if (effect_ == nullptr) {
        return fail(PipelineErrorCode::Internal, "VsrEffect: moved-from effect");
    }
    if (request.input_texture == 0 || request.output_surface == 0 ||
        request.input_size_px.width == 0 || request.input_size_px.height == 0 ||
        request.output_size_px.width == 0 || request.output_size_px.height == 0 ||
        request.quality > UpscaleQuality::Ultra) {
        return fail(PipelineErrorCode::InvalidParams, "VsrEffect: empty image or bad quality");
    }
    auto params = sized<rvb_vsr_eval_params>();
    params.quality = static_cast<std::uint32_t>(request.quality);
    params.input_texture = request.input_texture;
    params.output_surface = request.output_surface;
    params.input_rect = {.x = 0,
                         .y = 0,
                         .width = request.input_size_px.width,
                         .height = request.input_size_px.height};
    params.output_rect = {.x = 0,
                          .y = 0,
                          .width = request.output_size_px.width,
                          .height = request.output_size_px.height};
    const rvb_result r = api_.vsr_evaluate(effect_, &params);
    if (succeeded(r)) {
        return {};
    }
    return enh::Unexpected<PipelineError>(bridgeFailure(r, "rvb_vsr_evaluate"));
}

} // namespace enh::pipeline::rtxvideo
