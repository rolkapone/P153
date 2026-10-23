#pragma once

// Confiance dans les DLL NVIDIA (SEC-01, SEC-07, SEC-08 ; docs/gpu/ENH-040.md § 4) : chemins
// (modules charges par Windows, comparaison de chemins finaux), contenu identique, signature
// Authenticode de « NVIDIA Corporation » verifiee SANS reseau (aucune CRL / OCSP, SEC-07).
// Les messages d'erreur ne contiennent aucun chemin (journal, SEC-14).

#include "enh/pipeline/error.hpp"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace enh::pipeline::rtxvideo {

/// Forme de comparaison purement lexicale : prefixes \\?\ et \\?\UNC\ retires, '/' -> '\',
/// segments . et .. resolus, majuscules (comparaison insensible a la casse, ordinale).
[[nodiscard]] std::wstring lexicalPathKey(std::wstring_view path);

/// Chemin final du fichier (GetFinalPathNameByHandleW : liens symboliques et jonctions resolus) ;
/// a defaut (fichier absent...), le chemin tel quel.
[[nodiscard]] std::filesystem::path finalPath(const std::filesystem::path& path);

/// Meme fichier ? Compare les chemins finaux, sans tenir compte de la casse ni des prefixes.
[[nodiscard]] bool sameFile(const std::filesystem::path& a, const std::filesystem::path& b);

/// Chemins de tous les modules du processus dont le nom de fichier est `file_name` (casse ignoree).
[[nodiscard]] std::vector<std::filesystem::path> loadedModulePaths(std::wstring_view file_name);

/// Contenu octet par octet identique ? SourceUnreadable si un fichier ne peut etre lu.
[[nodiscard]] Result<bool> sameContent(const std::filesystem::path& a,
                                       const std::filesystem::path& b);

/// Verifie la signature Authenticode integree du fichier, hors ligne (WTD_REVOKE_NONE +
/// WTD_CACHE_ONLY_URL_RETRIEVAL) et renvoie le nom du signataire. Fichier non signe, signature
/// invalide ou chaine non reconnue -> ComponentMissing{RtxVideoSdk} (native_code = HRESULT).
[[nodiscard]] Result<std::wstring> verifyAuthenticodeOffline(const std::filesystem::path& file);

inline constexpr std::wstring_view kNvidiaSigner = L"NVIDIA Corporation";

/// verifyAuthenticodeOffline + signataire == « NVIDIA Corporation ».
[[nodiscard]] Result<void> requireNvidiaSignature(const std::filesystem::path& file);

/// Empreinte Authenticode SHA-256 du fichier (hachage PE hors signature, calcule localement par
/// CryptCATAdminCalcHashFromFileHandle2), en hexadecimal minuscule. Sert a journaliser une DLL
/// sans son chemin (SEC-14).
[[nodiscard]] Result<std::string> authenticodeSha256(const std::filesystem::path& file);

} // namespace enh::pipeline::rtxvideo
