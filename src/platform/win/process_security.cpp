#include "enh/platform/process_security.hpp"

#include "win_include.hpp"

#include <psapi.h>
#include <shlobj.h>

#include <algorithm>
#include <cwctype>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>

namespace enh::platform {
namespace {

// RAII pour un HANDLE Win32 : CloseHandle automatique (equivalent C : un goto cleanup).
struct HandleCloser {
    void operator()(HANDLE h) const noexcept {
        if (h != nullptr && h != INVALID_HANDLE_VALUE) {
            ::CloseHandle(h);
        }
    }
};
using UniqueHandle = std::unique_ptr<std::remove_pointer_t<HANDLE>, HandleCloser>;

// Appelle `fill(buffer, capacity)` (API Win32 a tampon de taille fixe) en agrandissant le tampon
// tant que le resultat est tronque : gere les chemins de plus de MAX_PATH caracteres.
template <typename Fill> std::wstring readGrowingString(Fill fill) {
    std::wstring buffer(MAX_PATH, L'\0');
    for (int attempt = 0; attempt < 8; ++attempt) {
        const DWORD written = fill(buffer.data(), static_cast<DWORD>(buffer.size()));
        if (written == 0) {
            return {};
        }
        if (written < buffer.size()) {
            buffer.resize(written);
            return buffer;
        }
        buffer.resize(buffer.size() * 2);
    }
    return {};
}

std::wstring lowerCase(std::wstring s) {
    std::ranges::transform(s, s.begin(),
                           [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
    return s;
}

// Chemin normalise, en minuscules, avec un separateur final : "c:\windows\system32\".
std::wstring normalizedDirectory(const std::filesystem::path& dir) {
    std::wstring s = lowerCase(dir.lexically_normal().make_preferred().wstring());
    if (!s.empty() && s.back() != L'\\') {
        s.push_back(L'\\');
    }
    return s;
}

bool isUnder(const std::wstring& normalizedFile, const std::filesystem::path& root) {
    if (root.empty()) {
        return false;
    }
    const std::wstring prefix = normalizedDirectory(root);
    return normalizedFile.size() > prefix.size() && normalizedFile.starts_with(prefix);
}

} // namespace

bool hardenDllSearchPath() noexcept {
    const BOOL defaults = ::SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_APPLICATION_DIR |
                                                     LOAD_LIBRARY_SEARCH_SYSTEM32);
    const BOOL noCurrentDir = ::SetDllDirectoryW(L"");
    return defaults != FALSE && noCurrentDir != FALSE;
}

bool isProcessElevated() noexcept {
    HANDLE raw = nullptr;
    if (::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &raw) == FALSE) {
        return false;
    }
    const UniqueHandle token(raw);
    TOKEN_ELEVATION elevation{};
    DWORD size = 0;
    if (::GetTokenInformation(token.get(), TokenElevation, &elevation, sizeof(elevation), &size) ==
        FALSE) {
        return false;
    }
    return elevation.TokenIsElevated != 0;
}

std::filesystem::path executableDirectory() {
    const std::wstring exe = readGrowingString(
        [](wchar_t* buf, DWORD cap) { return ::GetModuleFileNameW(nullptr, buf, cap); });
    return std::filesystem::path(exe).parent_path();
}

std::filesystem::path systemDirectory() {
    return readGrowingString([](wchar_t* buf, DWORD cap) {
        const UINT n = ::GetSystemDirectoryW(buf, cap);
        return n > cap ? cap : static_cast<DWORD>(n); // n > cap : tampon trop petit
    });
}

std::filesystem::path windowsDirectory() {
    return readGrowingString([](wchar_t* buf, DWORD cap) {
        const UINT n = ::GetWindowsDirectoryW(buf, cap);
        return n > cap ? cap : static_cast<DWORD>(n);
    });
}

std::vector<std::filesystem::path> loadedModulePaths() {
    void* const process = ::GetCurrentProcess(); // pseudo-handle HANDLE : rien a fermer
    std::vector<HMODULE> modules(256);
    for (int attempt = 0; attempt < 4; ++attempt) {
        DWORD needed = 0;
        const auto bytes = static_cast<DWORD>(modules.size() * sizeof(HMODULE));
        if (::EnumProcessModulesEx(process, modules.data(), bytes, &needed, LIST_MODULES_ALL) ==
            FALSE) {
            return {};
        }
        const std::size_t count = needed / sizeof(HMODULE);
        if (count <= modules.size()) {
            modules.resize(count);
            break;
        }
        modules.resize(count + 32); // des DLL ont ete chargees entre deux appels
    }

    std::vector<std::filesystem::path> paths;
    paths.reserve(modules.size());
    for (HMODULE m : modules) {
        std::wstring name = readGrowingString([process, m](wchar_t* buf, DWORD cap) {
            return ::GetModuleFileNameExW(process, m, buf, cap);
        });
        if (!name.empty()) {
            paths.emplace_back(std::move(name));
        }
    }
    return paths;
}

ModuleOrigin classifyModulePath(const std::filesystem::path& module,
                                const TrustedModuleRoots& roots) {
    // Prefixe de chemin long \\?\ retire pour comparer des formes identiques.
    constexpr std::wstring_view kLongPathPrefix = LR"(\\?\)";
    std::wstring raw = module.wstring();
    if (raw.starts_with(kLongPathPrefix)) {
        raw.erase(0, kLongPathPrefix.size());
    }
    const std::wstring file =
        lowerCase(std::filesystem::path(raw).lexically_normal().make_preferred().wstring());
    if (isUnder(file, roots.applicationDir) || isUnder(file, roots.system32Dir) ||
        isUnder(file, roots.winSxsDir)) {
        return ModuleOrigin::Trusted;
    }
    for (const auto& dir : roots.protectedThirdPartyDirs) {
        if (isUnder(file, dir)) {
            return ModuleOrigin::ProtectedThirdParty;
        }
    }
    return ModuleOrigin::Untrusted;
}

bool isTrustedModulePath(const std::filesystem::path& module, const TrustedModuleRoots& roots) {
    return classifyModulePath(module, roots) == ModuleOrigin::Trusted;
}

std::vector<std::filesystem::path> loadedModulesWithOrigin(const TrustedModuleRoots& roots,
                                                           ModuleOrigin origin) {
    std::vector<std::filesystem::path> result;
    for (auto& path : loadedModulePaths()) {
        if (classifyModulePath(path, roots) == origin) {
            result.push_back(std::move(path));
        }
    }
    return result;
}

TrustedModuleRoots currentTrustedModuleRoots() {
    TrustedModuleRoots roots{.applicationDir = executableDirectory(),
                             .system32Dir = systemDirectory(),
                             .winSxsDir = windowsDirectory() / L"WinSxS",
                             .protectedThirdPartyDirs = {}};
    for (const KNOWNFOLDERID& id : {FOLDERID_ProgramFiles, FOLDERID_ProgramFilesX86}) {
        PWSTR raw = nullptr;
        const HRESULT hr = ::SHGetKnownFolderPath(id, KF_FLAG_DEFAULT, nullptr, &raw);
        // RAII : CoTaskMemFree appele quoi qu'il arrive (meme en cas d'echec, raw peut etre
        // alloue).
        const std::unique_ptr<wchar_t, decltype(&::CoTaskMemFree)> owned(raw, &::CoTaskMemFree);
        if (SUCCEEDED(hr) && owned) {
            roots.protectedThirdPartyDirs.emplace_back(owned.get());
        }
    }
    return roots;
}

} // namespace enh::platform
