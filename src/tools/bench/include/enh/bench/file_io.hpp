#pragma once

// Fichiers du banc : lecture bornee d'une entree (Win32File, chemins etendus) et fichier de
// sortie RAII cree en CREATE_NEW (refus si existant, sauf `overwrite`).

#include "enh/expected.hpp"
#include "enh/pipeline/error.hpp"

#include <cstddef>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace enh::bench {

/// Lit tout le fichier ; UnsupportedFormat s'il depasse `max_bytes`, SourceCorrupt s'il est vide.
[[nodiscard]] pipeline::Result<std::vector<std::byte>>
readFileBounded(const std::filesystem::path& path, std::size_t max_bytes);

/// Fichier de sortie, comme une transaction : ouvert des le debut (echec rapide), ecrit en une
/// fois, puis VALIDE par commit(). Sans commit() (echec, exception, retour premature), le
/// destructeur SUPPRIME le fichier : jamais de sortie vide ou incomplete. Deplacable, non copiable.
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
    ~OutputFile(); // discard() si commit() n'a pas ete appele

    /// Remplace le contenu par `bytes` (retour au debut, ecriture, fin du fichier fixee).
    [[nodiscard]] pipeline::Result<void> replaceContent(std::span<const std::byte> bytes);
    [[nodiscard]] pipeline::Result<void> replaceContent(std::string_view text);
    /// Garde le fichier et le ferme. Sans effet si deja ferme.
    void commit() noexcept;
    /// Supprime le fichier et le ferme. Sans effet si deja ferme.
    void discard() noexcept;

private:
    explicit OutputFile(void* handle) noexcept : handle_(handle) {}
    void close() noexcept;

    void* handle_ = nullptr; // HANDLE ; nullptr = ferme (valide ou supprime)
};

/// Vrai si `a` et `b` designent le meme fichier : identite du fichier (std::filesystem::equivalent)
/// si les deux existent, sinon chemins normalises (weakly_canonical) compares sans tenir compte de
/// la casse (NTFS). Exemples : ".\a.bmp" et "a.bmp", "A.BMP" et "a.bmp", un lien physique.
[[nodiscard]] bool sameFile(const std::filesystem::path& a, const std::filesystem::path& b);

/// Refuse (message francais) si --in, --out et --report designent deux fois le meme fichier :
/// un outil de mesure ne doit jamais ecraser son entree, meme avec --force.
[[nodiscard]] enh::Expected<void, std::string>
checkDistinctFiles(const std::filesystem::path& input,
                   const std::optional<std::filesystem::path>& output,
                   const std::optional<std::filesystem::path>& report);

/// Memoire privee du processus (GetProcessMemoryInfo, PrivateUsage), en Mio.
[[nodiscard]] double processPrivateMib() noexcept;

/// Remplace dans `text` toute occurrence du dossier parent des chemins donnes (formes native,
/// generique et etendue \\?\) par "<dir>" : le JSON ne doit contenir aucun chemin complet.
[[nodiscard]] std::string redactPaths(std::string text,
                                      std::span<const std::filesystem::path> paths);

/// Nom du fichier en UTF-8 (sans dossier).
[[nodiscard]] std::string fileNameUtf8(const std::filesystem::path& path);

} // namespace enh::bench
