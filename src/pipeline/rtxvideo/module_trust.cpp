#include "enh/pipeline/rtxvideo/module_trust.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include <utility>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
// <windows.h> doit preceder les autres en-tetes Win32 : ordre protege de clang-format.
// clang-format off
#include <windows.h>
#include <psapi.h>
#include <softpub.h>
#include <wincrypt.h>
#include <wintrust.h>
#include <mscat.h>
// clang-format on

namespace enh::pipeline::rtxvideo {

namespace fs = std::filesystem;

namespace {

PipelineError untrusted(std::string detail, std::int64_t native = 0,
                        std::source_location where = std::source_location::current()) {
    PipelineError e =
        makeError(PipelineErrorCode::ComponentMissing, std::move(detail), native, where);
    e.component = Component::RtxVideoSdk;
    return e;
}

std::wstring upperInvariant(const std::wstring& s) {
    if (s.empty()) {
        return s;
    }
    std::wstring out(s.size(), L'\0');
    const int n =
        LCMapStringEx(LOCALE_NAME_INVARIANT, LCMAP_UPPERCASE, s.c_str(), static_cast<int>(s.size()),
                      out.data(), static_cast<int>(out.size()), nullptr, nullptr, 0);
    if (n <= 0) {
        return s;
    }
    out.resize(static_cast<std::size_t>(n));
    return out;
}

struct HandleCloser {
    void operator()(HANDLE h) const noexcept {
        if (h != nullptr && h != INVALID_HANDLE_VALUE) {
            CloseHandle(h);
        }
    }
};
using UniqueHandle = std::unique_ptr<void, HandleCloser>;

/// Libere l'etat d'une verification WinVerifyTrust (pas la structure, qui est sur la pile).
struct TrustStateCloser {
    void operator()(WINTRUST_DATA* data) const noexcept {
        GUID action = WINTRUST_ACTION_GENERIC_VERIFY_V2;
        data->dwStateAction = WTD_STATEACTION_CLOSE;
        // NOLINTNEXTLINE(performance-no-int-to-ptr)
        (void)WinVerifyTrust(static_cast<HWND>(INVALID_HANDLE_VALUE), &action, data);
    }
};

std::wstring fileNameKey(const fs::path& p) {
    return upperInvariant(p.filename().wstring());
}

} // namespace

std::wstring lexicalPathKey(std::wstring_view path) {
    std::wstring s(path);
    std::ranges::replace(s, L'/', L'\\');
    constexpr std::wstring_view kUnc = L"\\\\?\\UNC\\";
    constexpr std::wstring_view kLong = L"\\\\?\\";
    if (s.starts_with(kUnc)) {
        s = L"\\\\" + s.substr(kUnc.size());
    } else if (s.starts_with(kLong)) {
        s = s.substr(kLong.size());
    }
    s = fs::path(s).lexically_normal().wstring();
    while (s.size() > 3 && s.back() == L'\\') {
        s.pop_back();
    }
    return upperInvariant(std::move(s));
}

fs::path finalPath(const fs::path& path) {
    const UniqueHandle h(CreateFileW(path.c_str(), FILE_READ_ATTRIBUTES,
                                     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                     nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr));
    if (h.get() == INVALID_HANDLE_VALUE) {
        return path;
    }
    std::wstring buf(MAX_PATH, L'\0');
    for (int attempt = 0; attempt < 2; ++attempt) {
        const DWORD n =
            GetFinalPathNameByHandleW(h.get(), buf.data(), static_cast<DWORD>(buf.size()),
                                      FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
        if (n == 0) {
            return path;
        }
        if (n < buf.size()) {
            buf.resize(n);
            return {buf};
        }
        buf.assign(n + 1, L'\0'); // n = taille requise, zero final compris
    }
    return path;
}

bool sameFile(const fs::path& a, const fs::path& b) {
    return lexicalPathKey(finalPath(a).wstring()) == lexicalPathKey(finalPath(b).wstring());
}

std::vector<fs::path> loadedModulePaths(std::wstring_view file_name) {
    const std::wstring wanted = upperInvariant(std::wstring(file_name));
    std::vector<HMODULE> modules(256);
    for (;;) {
        DWORD needed = 0;
        const auto bytes = static_cast<DWORD>(modules.size() * sizeof(HMODULE));
        if (EnumProcessModulesEx(GetCurrentProcess(), modules.data(), bytes, &needed,
                                 LIST_MODULES_ALL) == FALSE) {
            return {};
        }
        if (needed <= bytes) {
            modules.resize(needed / sizeof(HMODULE));
            break;
        }
        modules.resize(needed / sizeof(HMODULE) + 16); // modules charges entre-temps
    }
    std::vector<fs::path> out;
    std::wstring buf(32768, L'\0');
    for (HMODULE m : modules) {
        const DWORD n = GetModuleFileNameW(m, buf.data(), static_cast<DWORD>(buf.size()));
        if (n == 0 || n >= buf.size()) {
            continue;
        }
        fs::path p(std::wstring(buf.data(), n));
        if (fileNameKey(p) == wanted) {
            out.push_back(std::move(p));
        }
    }
    return out;
}

Result<bool> sameContent(const fs::path& a, const fs::path& b) {
    std::error_code ec;
    const auto size_a = fs::file_size(a, ec);
    if (ec) {
        return fail(PipelineErrorCode::SourceUnreadable, "sameContent: unreadable file",
                    ec.value());
    }
    const auto size_b = fs::file_size(b, ec);
    if (ec) {
        return fail(PipelineErrorCode::SourceUnreadable, "sameContent: unreadable file",
                    ec.value());
    }
    if (size_a != size_b) {
        return false;
    }
    std::ifstream fa(a, std::ios::binary);
    std::ifstream fb(b, std::ios::binary);
    if (!fa || !fb) {
        return fail(PipelineErrorCode::SourceUnreadable, "sameContent: cannot open file");
    }
    constexpr std::size_t kChunk = std::size_t{1} << 20;
    std::vector<char> ba(kChunk);
    std::vector<char> bb(kChunk);
    while (fa && fb) {
        fa.read(ba.data(), static_cast<std::streamsize>(kChunk));
        fb.read(bb.data(), static_cast<std::streamsize>(kChunk));
        if (fa.gcount() != fb.gcount() ||
            !std::equal(ba.begin(), ba.begin() + fa.gcount(), bb.begin())) {
            return false;
        }
    }
    return true;
}

Result<std::wstring> verifyAuthenticodeOffline(const fs::path& file) {
    const std::wstring path = file.wstring();
    WINTRUST_FILE_INFO file_info{};
    file_info.cbStruct = sizeof(file_info);
    file_info.pcwszFilePath = path.c_str();

    WINTRUST_DATA data{};
    data.cbStruct = sizeof(data);
    data.dwUIChoice = WTD_UI_NONE;
    data.fdwRevocationChecks = WTD_REVOKE_NONE; // SEC-07 : pas de CRL / OCSP
    data.dwUnionChoice = WTD_CHOICE_FILE;
    data.pFile = &file_info;
    data.dwStateAction = WTD_STATEACTION_VERIFY;
    // Hors ligne : chaine construite avec le seul cache local, aucune URL recuperee.
    data.dwProvFlags =
        WTD_CACHE_ONLY_URL_RETRIEVAL | WTD_REVOCATION_CHECK_NONE | WTD_DISABLE_MD2_MD4;

    GUID action = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    // NOLINTNEXTLINE(performance-no-int-to-ptr) : INVALID_HANDLE_VALUE = aucune interface
    const LONG status = WinVerifyTrust(static_cast<HWND>(INVALID_HANDLE_VALUE), &action, &data);
    // L'etat est toujours libere (WTD_STATEACTION_CLOSE), meme en cas d'echec.
    const std::unique_ptr<WINTRUST_DATA, TrustStateCloser> closer(&data);

    if (status != ERROR_SUCCESS) {
        return enh::Unexpected<PipelineError>(
            untrusted("Authenticode verification failed", static_cast<std::int64_t>(status)));
    }
    CRYPT_PROVIDER_DATA* provider = WTHelperProvDataFromStateData(data.hWVTStateData);
    CRYPT_PROVIDER_SGNR* signer =
        provider != nullptr ? WTHelperGetProvSignerFromChain(provider, 0, FALSE, 0) : nullptr;
    const CRYPT_PROVIDER_CERT* cert =
        signer != nullptr ? WTHelperGetProvCertFromChain(signer, 0) : nullptr;
    if (cert == nullptr || cert->pCert == nullptr) {
        return enh::Unexpected<PipelineError>(untrusted("Authenticode: no signer certificate"));
    }
    std::wstring name(256, L'\0');
    const DWORD n = CertGetNameStringW(cert->pCert, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, nullptr,
                                       name.data(), static_cast<DWORD>(name.size()));
    if (n <= 1) {
        return enh::Unexpected<PipelineError>(untrusted("Authenticode: unnamed signer"));
    }
    name.resize(n - 1); // n compte le zero final
    return name;
}

Result<void> requireNvidiaSignature(const fs::path& file) {
    auto signer = verifyAuthenticodeOffline(file);
    if (!signer) {
        return enh::Unexpected<PipelineError>(std::move(signer.error()));
    }
    if (*signer != kNvidiaSigner) {
        return enh::Unexpected<PipelineError>(untrusted("Authenticode: signer is not NVIDIA"));
    }
    return {};
}

Result<std::string> authenticodeSha256(const fs::path& file) {
    HCATADMIN admin = nullptr;
    if (CryptCATAdminAcquireContext2(&admin, nullptr, L"SHA256", nullptr, 0) == FALSE) {
        return fail(PipelineErrorCode::Internal, "CryptCATAdminAcquireContext2 failed",
                    GetLastError());
    }
    struct AdminCloser {
        void operator()(HCATADMIN h) const noexcept { CryptCATAdminReleaseContext(h, 0); }
    };
    const std::unique_ptr<void, AdminCloser> admin_guard(admin);
    const UniqueHandle h(CreateFileW(file.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                                     OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
    if (h.get() == INVALID_HANDLE_VALUE) {
        return fail(PipelineErrorCode::SourceUnreadable, "authenticodeSha256: cannot open file",
                    GetLastError());
    }
    std::array<BYTE, 64> hash{};
    auto size = static_cast<DWORD>(hash.size());
    if (CryptCATAdminCalcHashFromFileHandle2(admin, h.get(), &size, hash.data(), 0) == FALSE ||
        size > hash.size()) {
        return fail(PipelineErrorCode::Internal, "CryptCATAdminCalcHashFromFileHandle2 failed",
                    GetLastError());
    }
    constexpr std::string_view kDigits = "0123456789abcdef";
    std::string out;
    out.reserve(std::size_t{size} * 2);
    for (DWORD i = 0; i < size; ++i) {
        out += kDigits[hash.at(i) >> 4U];
        out += kDigits[hash.at(i) & 0xFU];
    }
    return out;
}

} // namespace enh::pipeline::rtxvideo
