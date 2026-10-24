#pragma once

// Fichiers du banc : lecture bornee d'une entree (Win32File, chemins etendus) et fichier de
// sortie RAII cree en CREATE_NEW (refus si existant, sauf `overwrite`).

#include "enh/pipeline/error.hpp"

#include <cstddef>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace enh::bench {

/// Lit tout le fichier ; UnsupportedFormat s'il depasse `max_bytes`, SourceCorrupt s'il est vide.
[[nodiscard]] pipeline::Result<std::vector<std::byte>>
readFileBounded(const std::filesystem::path& path, std::size_t max_bytes);

/// Fichier de sortie : ouvert des le debut (echec rapide), ecrit en une fois, ferme par le
/// destructeur (CloseHandle). Deplacable, non copiable.
class OutputFile {
public:
    /// OutputExists si le fichier existe et `overwrite` est faux ; WriteDenied sinon en cas
    /// d'echec.
    [[nodiscard]] static pipeline::Result<OutputFile> create(const std::filesystem::path& path,
                                                             bool overwrite);

    OutputFile(OutputFile&& other) noexcept;
    OutputFile& operator=(OutputFile&& other) noexcept;
    OutputFile(const OutputFile&) = delete;
    OutputFile& operator=(const OutputFile&) = delete;
    ~OutputFile();

    /// Remplace le contenu par `bytes` (retour au debut, ecriture, fin du fichier fixee).
    [[nodiscard]] pipeline::Result<void> replaceContent(std::span<const std::byte> bytes);
    [[nodiscard]] pipeline::Result<void> replaceContent(std::string_view text);
    /// Supprime le fichier (sortie incomplete apres un echec) et le ferme. Sans effet si ferme.
    void discard() noexcept;

private:
    explicit OutputFile(void* handle) noexcept : handle_(handle) {}
    void close() noexcept;

    void* handle_ = nullptr; // HANDLE ; nullptr = ferme
};

/// Memoire privee du processus (GetProcessMemoryInfo, PrivateUsage), en Mio.
[[nodiscard]] double processPrivateMib() noexcept;

/// Remplace dans `text` toute occurrence du dossier parent des chemins donnes (formes native,
/// generique et etendue \\?\) par "<dir>" : le JSON ne doit contenir aucun chemin complet.
[[nodiscard]] std::string redactPaths(std::string text,
                                      std::span<const std::filesystem::path> paths);

/// Nom du fichier en UTF-8 (sans dossier).
[[nodiscard]] std::string fileNameUtf8(const std::filesystem::path& path);

} // namespace enh::bench
