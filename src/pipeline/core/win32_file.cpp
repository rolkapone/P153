#include "enh/pipeline/core/win32_file.hpp"

#include <limits>
#include <system_error>
#include <utility>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace enh::pipeline {

namespace {

std::string win32Message(DWORD err) {
    // Message systeme en anglais neutre (journal) : code + texte de std::system_category.
    return "win32 error " + std::to_string(err) + " (" +
           std::system_category().message(static_cast<int>(err)) + ")";
}

HANDLE asHandle(void* h) noexcept {
    return static_cast<HANDLE>(h);
}

} // namespace

std::wstring toExtendedPath(const std::filesystem::path& path) {
    std::wstring raw = path.native();
    if (raw.starts_with(LR"(\\?\)")) {
        return raw;
    }
    std::error_code ec;
    std::filesystem::path abs = std::filesystem::absolute(path, ec);
    if (ec) {
        abs = path;
    }
    std::wstring full = abs.lexically_normal().native();
    for (auto& c : full) {
        if (c == L'/') {
            c = L'\\';
        }
    }
    if (full.starts_with(LR"(\\)")) {
        // \\serveur\partage\... -> \\?\UNC\serveur\partage\...
        return LR"(\\?\UNC\)" + full.substr(2);
    }
    return LR"(\\?\)" + full;
}

PipelineErrorCode mapSourceWin32Error(unsigned long win32_error) noexcept {
    switch (win32_error) {
    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
    case ERROR_INVALID_DRIVE:
    case ERROR_BAD_NETPATH:
    case ERROR_BAD_NET_NAME:
    case ERROR_NETNAME_DELETED:
    case ERROR_DEV_NOT_EXIST:
    case ERROR_NOT_READY:
    case ERROR_UNEXP_NET_ERR:
        return PipelineErrorCode::SourceMissing;
    default:
        return PipelineErrorCode::SourceUnreadable;
    }
}

Result<Win32File> Win32File::openRead(const std::filesystem::path& path) {
    const std::wstring extended = toExtendedPath(path);
    HANDLE h = CreateFileW(extended.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        const DWORD err = GetLastError();
        return fail(mapSourceWin32Error(err), "CreateFileW failed: " + win32Message(err),
                    static_cast<std::int64_t>(err));
    }
    Win32File file(h); // possede le HANDLE des maintenant (ferme meme si on refuse ci-dessous)
    if (GetFileType(h) != FILE_TYPE_DISK) {
        return fail(PipelineErrorCode::SourceUnreadable, "not a disk file");
    }
    return file;
}

Win32File::Win32File(Win32File&& other) noexcept : handle_(std::exchange(other.handle_, nullptr)) {}

Win32File& Win32File::operator=(Win32File&& other) noexcept {
    if (this != &other) {
        close();
        handle_ = std::exchange(other.handle_, nullptr);
    }
    return *this;
}

Win32File::~Win32File() {
    close();
}

void Win32File::close() noexcept {
    if (handle_ != nullptr) {
        CloseHandle(asHandle(handle_));
        handle_ = nullptr;
    }
}

bool Win32File::isOpen() const noexcept {
    return handle_ != nullptr;
}

Result<std::size_t> Win32File::read(std::span<std::byte> buffer) noexcept {
    if (handle_ == nullptr) {
        return fail(PipelineErrorCode::Internal, "read on closed file");
    }
    const DWORD want = buffer.size() > std::numeric_limits<DWORD>::max()
                           ? std::numeric_limits<DWORD>::max()
                           : static_cast<DWORD>(buffer.size());
    DWORD got = 0;
    if (ReadFile(asHandle(handle_), buffer.data(), want, &got, nullptr) == FALSE) {
        const DWORD err = GetLastError();
        if (err == ERROR_HANDLE_EOF) {
            return std::size_t{0};
        }
        return fail(mapSourceWin32Error(err), "ReadFile failed: " + win32Message(err),
                    static_cast<std::int64_t>(err));
    }
    return static_cast<std::size_t>(got);
}

Result<std::int64_t> Win32File::seek(std::int64_t offset, SeekOrigin origin) noexcept {
    if (handle_ == nullptr) {
        return fail(PipelineErrorCode::Internal, "seek on closed file");
    }
    DWORD method = FILE_BEGIN;
    if (origin == SeekOrigin::Current) {
        method = FILE_CURRENT;
    } else if (origin == SeekOrigin::End) {
        method = FILE_END;
    }
    LARGE_INTEGER dist{};
    dist.QuadPart = offset;
    LARGE_INTEGER pos{};
    if (SetFilePointerEx(asHandle(handle_), dist, &pos, method) == FALSE) {
        const DWORD err = GetLastError();
        return fail(mapSourceWin32Error(err), "SetFilePointerEx failed: " + win32Message(err),
                    static_cast<std::int64_t>(err));
    }
    return static_cast<std::int64_t>(pos.QuadPart);
}

Result<std::int64_t> Win32File::size() const noexcept {
    if (handle_ == nullptr) {
        return fail(PipelineErrorCode::Internal, "size on closed file");
    }
    LARGE_INTEGER sz{};
    if (GetFileSizeEx(asHandle(handle_), &sz) == FALSE) {
        const DWORD err = GetLastError();
        return fail(mapSourceWin32Error(err), "GetFileSizeEx failed: " + win32Message(err),
                    static_cast<std::int64_t>(err));
    }
    return static_cast<std::int64_t>(sz.QuadPart);
}

} // namespace enh::pipeline
