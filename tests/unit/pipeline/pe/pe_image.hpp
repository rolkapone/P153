#pragma once

// Lecture minimale d'un fichier PE32+ (x64) pour les tests : DLL importees (imports normaux et
// differes) et noms exportes. Equivalent de `dumpbin /dependents` et `dumpbin /exports`, sans
// dependre des outils MSVC au moment du test (critere ENH-040 : aucun cudart*.dll importe).

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace enh::tests {

struct PeInfo {
    std::vector<std::string> imports;       // noms de DLL, tels qu'ecrits dans le fichier
    std::vector<std::string> delay_imports; // idem, imports differes
    std::vector<std::string> exports;       // noms exportes (pas les ordinaux seuls)
};

/// nullopt si le fichier est illisible ou n'est pas un PE32+ coherent (bornes verifiees).
[[nodiscard]] std::optional<PeInfo> readPe(const std::filesystem::path& file);

} // namespace enh::tests
