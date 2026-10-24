#include "enh/pipeline/rtxvideo/bridge_library.hpp"

#include <string>
#include <utility>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace enh::pipeline::rtxvideo {

namespace {

PipelineError missing(std::string detail, std::int64_t native = 0,
                      std::source_location where = std::source_location::current()) {
    PipelineError e =
        makeError(PipelineErrorCode::ComponentMissing, std::move(detail), native, where);
    e.component = Component::RtxVideoSdk;
    return e;
}

/// GetProcAddress type : la conversion FARPROC -> pointeur de fonction est le seul cast possible.
template <typename Fn> Fn symbol(HMODULE module, const char* name) noexcept {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return reinterpret_cast<Fn>(GetProcAddress(module, name));
}

} // namespace

void BridgeLibrary::ModuleDeleter::operator()(void* module) const noexcept {
    if (module != nullptr) {
        FreeLibrary(static_cast<HMODULE>(module));
    }
}

Result<std::filesystem::path> applicationDirectory() {
    std::vector<wchar_t> buf(MAX_PATH);
    for (;;) {
        const DWORD n = GetModuleFileNameW(nullptr, buf.data(), static_cast<DWORD>(buf.size()));
        if (n == 0) {
            return fail(PipelineErrorCode::Internal, "GetModuleFileNameW failed", GetLastError());
        }
        if (n < buf.size()) {
            return std::filesystem::path(std::wstring(buf.data(), n)).parent_path();
        }
        if (buf.size() >= 32768) {
            return fail(PipelineErrorCode::Internal, "GetModuleFileNameW: path too long");
        }
        buf.resize(buf.size() * 2);
    }
}

Result<BridgeLibrary> BridgeLibrary::load(const std::filesystem::path& dll_path) {
    if (!dll_path.is_absolute()) {
        return fail(PipelineErrorCode::InvalidParams, "bridge DLL path must be absolute");
    }
    // Chemin absolu : seules les dependances sont cherchees, dans le dossier de la DLL puis
    // System32 (jamais le dossier courant ni le PATH, SEC-08).
    HMODULE raw = LoadLibraryExW(dll_path.c_str(), nullptr,
                                 LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (raw == nullptr) {
        const DWORD err = GetLastError();
        return enh::Unexpected<PipelineError>(
            missing("enh_rtxvideo_bridge.dll could not be loaded", static_cast<std::int64_t>(err)));
    }
    std::unique_ptr<void, ModuleDeleter> module(raw);

    BridgeApi api;
    api.abi_version = symbol<rvb_abi_version_fn>(raw, "rvb_abi_version");
    api.runtime_create = symbol<rvb_runtime_create_fn>(raw, "rvb_runtime_create");
    api.runtime_destroy = symbol<rvb_runtime_destroy_fn>(raw, "rvb_runtime_destroy");
    api.vsr_query = symbol<rvb_vsr_query_fn>(raw, "rvb_vsr_query");
    api.vsr_create = symbol<rvb_vsr_create_fn>(raw, "rvb_vsr_create");
    api.vsr_evaluate = symbol<rvb_vsr_evaluate_fn>(raw, "rvb_vsr_evaluate");
    api.effect_destroy = symbol<rvb_effect_destroy_fn>(raw, "rvb_effect_destroy");
    if (!api.complete()) {
        return enh::Unexpected<PipelineError>(missing("enh_rtxvideo_bridge.dll: missing export"));
    }
    const std::uint32_t abi = api.abi_version();
    if (abi != RVB_ABI_VERSION) {
        return enh::Unexpected<PipelineError>(missing("enh_rtxvideo_bridge.dll: ABI " +
                                                          std::to_string(abi) + ", expected " +
                                                          std::to_string(RVB_ABI_VERSION),
                                                      abi));
    }
    return BridgeLibrary(std::move(module), api);
}

} // namespace enh::pipeline::rtxvideo
