#pragma once

// Fichier Win32 en lecture (ADR-049) : CreateFileW sur la forme etendue du chemin (\\?\ ou
// \\?\UNC\), aucun passage par UTF-8, aucune limite a 260 caracteres. Classe RAII : le
// destructeur ferme le HANDLE (CloseHandle). Pas de <windows.h> dans cet en-tete.

#include "enh/pipeline/error.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>

namespace enh::pipeline {

/// Chemin absolu normalise avec prefixe etendu : C:\a -> \\?\C:\a ; \\srv\share\a ->
/// \\?\UNC\srv\share\a ; un chemin deja prefixe est rendu tel quel.
[[nodiscard]] std::wstring toExtendedPath(const std::filesystem::path& path);

/// Erreur Win32 (GetLastError) d'ouverture ou de lecture d'une SOURCE -> code P-08 (ADR-049).
[[nodiscard]] PipelineErrorCode mapSourceWin32Error(unsigned long win32_error) noexcept;

enum class SeekOrigin : std::uint8_t { Begin, Current, End };

class Win32File {
public:
    /// GENERIC_READ, FILE_SHARE_READ, FILE_FLAG_SEQUENTIAL_SCAN ; refuse ce qui n'est pas un
    /// fichier disque (tube, peripherique, dossier).
    [[nodiscard]] static Result<Win32File> openRead(const std::filesystem::path& path);

    Win32File(Win32File&& other) noexcept;
    Win32File& operator=(Win32File&& other) noexcept;
    Win32File(const Win32File&) = delete;
    Win32File& operator=(const Win32File&) = delete;
    ~Win32File();

    /// Lit au plus buffer.size() octets ; 0 = fin du fichier.
    [[nodiscard]] Result<std::size_t> read(std::span<std::byte> buffer) noexcept;
    /// Nouvelle position absolue.
    [[nodiscard]] Result<std::int64_t> seek(std::int64_t offset, SeekOrigin origin) noexcept;
    [[nodiscard]] Result<std::int64_t> size() const noexcept;
    [[nodiscard]] bool isOpen() const noexcept;

private:
    explicit Win32File(void* handle) noexcept : handle_(handle) {}
    void close() noexcept;

    void* handle_ = nullptr; // HANDLE ; nullptr = ferme
};

} // namespace enh::pipeline
