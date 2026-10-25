#include "enh/bench/file_io.hpp"

#include "enh/pipeline/core/win32_file.hpp"

#include <limits>
#include <utility>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
// <windows.h> doit preceder <psapi.h> : ordre protege de clang-format.
// clang-format off
#include <windows.h>
#include <psapi.h>
// clang-format on

namespace enh::bench {

namespace {

using pipeline::fail;
using pipeline::PipelineErrorCode;
using pipeline::Result;

std::string toUtf8(const std::filesystem::path& p) {
    const std::u8string s = p.u8string();
    return {s.begin(), s.end()};
}

void replaceAll(std::string& text, const std::string& what, std::string_view by) {
    if (what.empty()) {
        return;
    }
    for (std::size_t pos = text.find(what); pos != std::string::npos;
         pos = text.find(what, pos + by.size())) {
        text.replace(pos, what.size(), by);
    }
}

} // namespace

Result<std::vector<std::byte>> readFileBounded(const std::filesystem::path& path,
                                               std::size_t max_bytes) {
    auto file = pipeline::Win32File::openRead(path);
    if (!file) {
        return enh::Unexpected<pipeline::PipelineError>(std::move(file.error()));
    }
    auto size = file->size();
    if (!size) {
        return enh::Unexpected<pipeline::PipelineError>(std::move(size.error()));
    }
    if (*size <= 0) {
        return fail(PipelineErrorCode::SourceCorrupt, "empty file");
    }
    if (std::cmp_greater(*size, max_bytes)) {
        return fail(PipelineErrorCode::UnsupportedFormat,
                    "file too large (" + std::to_string(*size) + " bytes, max " +
                        std::to_string(max_bytes) + ")");
    }
    std::vector<std::byte> data(static_cast<std::size_t>(*size));
    std::size_t done = 0;
    while (done < data.size()) {
        auto n = file->read(std::span<std::byte>(data).subspan(done));
        if (!n) {
            return enh::Unexpected<pipeline::PipelineError>(std::move(n.error()));
        }
        if (*n == 0) {
            return fail(PipelineErrorCode::SourceCorrupt, "file shrank while reading");
        }
        done += *n;
    }
    return data;
}

Result<OutputFile> OutputFile::create(const std::filesystem::path& path, bool overwrite) {
    const std::wstring ext = pipeline::toExtendedPath(path);
    HANDLE h = CreateFileW(ext.c_str(), GENERIC_WRITE | DELETE, 0, nullptr,
                           overwrite ? CREATE_ALWAYS : CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        const DWORD err = GetLastError();
        if (err == ERROR_FILE_EXISTS || err == ERROR_ALREADY_EXISTS) {
            return fail(PipelineErrorCode::OutputExists, "output file already exists (use --force)",
                        static_cast<std::int64_t>(err));
        }
        return fail(PipelineErrorCode::WriteDenied, "cannot create output file",
                    static_cast<std::int64_t>(err));
    }
    return OutputFile(h);
}

OutputFile::OutputFile(OutputFile&& other) noexcept
    : handle_(std::exchange(other.handle_, nullptr)) {}

OutputFile& OutputFile::operator=(OutputFile&& other) noexcept {
    if (this != &other) {
        discard(); // l'ancien fichier n'a pas ete valide
        handle_ = std::exchange(other.handle_, nullptr);
    }
    return *this;
}

OutputFile::~OutputFile() {
    discard(); // sans commit() : sortie supprimee (echec, exception, retour premature)
}

void OutputFile::commit() noexcept {
    close();
}

void OutputFile::close() noexcept {
    if (handle_ != nullptr) {
        CloseHandle(static_cast<HANDLE>(handle_));
        handle_ = nullptr;
    }
}

Result<void> OutputFile::replaceContent(std::span<const std::byte> bytes) {
    if (handle_ == nullptr) {
        return fail(PipelineErrorCode::Internal, "output file closed");
    }
    auto* h = static_cast<HANDLE>(handle_);
    if (SetFilePointerEx(h, LARGE_INTEGER{}, nullptr, FILE_BEGIN) == FALSE) {
        return fail(PipelineErrorCode::WriteDenied, "SetFilePointerEx failed",
                    static_cast<std::int64_t>(GetLastError()));
    }
    std::size_t done = 0;
    while (done < bytes.size()) {
        const auto chunk = static_cast<DWORD>(
            std::min<std::size_t>(bytes.size() - done, std::numeric_limits<DWORD>::max() / 2));
        DWORD written = 0;
        if (WriteFile(h, bytes.data() + done, chunk, &written, nullptr) == FALSE || written == 0) {
            const DWORD err = GetLastError();
            return fail(err == ERROR_DISK_FULL || err == ERROR_HANDLE_DISK_FULL
                            ? PipelineErrorCode::DiskFull
                            : PipelineErrorCode::WriteDenied,
                        "WriteFile failed", static_cast<std::int64_t>(err));
        }
        done += written;
    }
    if (SetEndOfFile(h) == FALSE) {
        return fail(PipelineErrorCode::WriteDenied, "SetEndOfFile failed",
                    static_cast<std::int64_t>(GetLastError()));
    }
    return {};
}

void OutputFile::discard() noexcept {
    if (handle_ == nullptr) {
        return;
    }
    FILE_DISPOSITION_INFO info{};
    info.DeleteFile = TRUE;
    (void)SetFileInformationByHandle(static_cast<HANDLE>(handle_), FileDispositionInfo, &info,
                                     sizeof info);
    close(); // suppression effective a la fermeture
}

Result<void> OutputFile::replaceContent(std::string_view text) {
    return replaceContent(std::as_bytes(std::span<const char>(text.data(), text.size())));
}

double processPrivateMib() noexcept {
    PROCESS_MEMORY_COUNTERS_EX pmc{};
    pmc.cb = sizeof pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc),
                             sizeof pmc) == FALSE) {
        return 0.0;
    }
    return static_cast<double>(pmc.PrivateUsage) / (1024.0 * 1024.0);
}

std::string redactPaths(std::string text, std::span<const std::filesystem::path> paths) {
    for (const std::filesystem::path& p : paths) {
        if (p.empty()) {
            continue;
        }
        std::error_code ec;
        const std::filesystem::path abs = std::filesystem::absolute(p, ec);
        for (const std::filesystem::path& candidate : {p, abs}) {
            const std::filesystem::path dir = candidate.parent_path();
            if (dir.empty()) {
                continue;
            }
            const std::string native = toUtf8(dir);
            const std::string generic = toUtf8(dir.generic_wstring());
            const std::string extended =
                toUtf8(std::filesystem::path(pipeline::toExtendedPath(candidate)).parent_path());
            // Le plus long d'abord : \\?\C:\a avant C:\a.
            replaceAll(text, extended, "<dir>");
            replaceAll(text, native, "<dir>");
            replaceAll(text, generic, "<dir>");
        }
    }
    return text;
}

std::string fileNameUtf8(const std::filesystem::path& path) {
    return toUtf8(path.filename());
}

bool sameFile(const std::filesystem::path& a, const std::filesystem::path& b) {
    namespace fs = std::filesystem;
    std::error_code ec_a;
    std::error_code ec_b;
    const bool a_exists = fs::exists(fs::path(pipeline::toExtendedPath(a)), ec_a);
    const bool b_exists = fs::exists(fs::path(pipeline::toExtendedPath(b)), ec_b);
    if (a_exists && b_exists) {
        std::error_code ec;
        const bool eq = fs::equivalent(fs::path(pipeline::toExtendedPath(a)),
                                       fs::path(pipeline::toExtendedPath(b)), ec);
        if (!ec) {
            return eq; // identite du fichier : couvre aussi les liens physiques
        }
    }
    std::error_code ec;
    const fs::path ca = fs::weakly_canonical(fs::absolute(a, ec), ec);
    const fs::path cb = fs::weakly_canonical(fs::absolute(b, ec), ec);
    const std::wstring& wa = ca.native();
    const std::wstring& wb = cb.native();
    // NTFS ne distingue pas la casse : comparaison ordinale insensible a la casse.
    return CompareStringOrdinal(wa.c_str(), static_cast<int>(wa.size()), wb.c_str(),
                                static_cast<int>(wb.size()), TRUE) == CSTR_EQUAL;
}

enh::Expected<void, std::string>
checkDistinctFiles(const std::filesystem::path& input,
                   const std::optional<std::filesystem::path>& output,
                   const std::optional<std::filesystem::path>& report) {
    using Error = enh::Unexpected<std::string>;
    if (output && sameFile(input, *output)) {
        return Error("--out designe le fichier d'entree : refuse, meme avec --force");
    }
    if (report && sameFile(input, *report)) {
        return Error("--report designe le fichier d'entree : refuse, meme avec --force");
    }
    if (output && report && sameFile(*output, *report)) {
        return Error("--report et --out designent le meme fichier");
    }
    return {};
}

} // namespace enh::bench
