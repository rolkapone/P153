#pragma once

// Durcissement du processus (SEC-08 chargement de DLL, SEC-20 execution en administrateur).
// Couche Win32 sans Qt.

#include <filesystem>
#include <vector>

namespace enh::platform {

/// SEC-08 : limite la recherche des DLL chargees dynamiquement au dossier de l'application et a
/// System32 (SetDefaultDllDirectories) et retire le dossier courant (SetDllDirectoryW(L"")).
/// A appeler en toute premiere instruction de main(). Renvoie false si un appel a echoue.
bool hardenDllSearchPath() noexcept;

/// SEC-20 : vrai si le jeton du processus est eleve (lance « en tant qu'administrateur »).
[[nodiscard]] bool isProcessElevated() noexcept;

/// Dossier de l'executable courant (chemins longs et Unicode geres).
[[nodiscard]] std::filesystem::path executableDirectory();

/// Dossier System32 (GetSystemDirectoryW).
[[nodiscard]] std::filesystem::path systemDirectory();

/// Dossier Windows (GetWindowsDirectoryW).
[[nodiscard]] std::filesystem::path windowsDirectory();

/// Chemins complets de tous les modules (exe et DLL) charges dans le processus courant.
[[nodiscard]] std::vector<std::filesystem::path> loadedModulePaths();

/// Dossiers de confiance pour le chargement des modules.
struct TrustedModuleRoots {
    std::filesystem::path applicationDir; ///< dossier de enhance.exe
    std::filesystem::path system32Dir;    ///< %SystemRoot%\System32 (DriverStore inclus)
    std::filesystem::path winSxsDir;      ///< %SystemRoot%\WinSxS (composants systeme)
    /// Dossiers « Program Files » : proteges en ecriture (administrateur). Un module qui en vient
    /// a ete injecte par un logiciel installe (antivirus, superposition graphique) : il est
    /// signale, mais ne releve pas du detournement de chemin de recherche vise par SEC-08.
    std::vector<std::filesystem::path> protectedThirdPartyDirs;
};

/// Origine d'un module charge.
enum class ModuleOrigin {
    Trusted,             ///< dossier de l'application, System32 ou WinSxS
    ProtectedThirdParty, ///< logiciel installe dans Program Files (injection externe)
    Untrusted,           ///< tout autre dossier : non conforme a SEC-08
};

/// Classe `module` (comparaison insensible a la casse, apres normalisation lexicale :
/// « ..\ » ne permet pas de sortir d'un dossier).
[[nodiscard]] ModuleOrigin classifyModulePath(const std::filesystem::path& module,
                                              const TrustedModuleRoots& roots);

/// Vrai si `module` est dans un des dossiers de confiance.
[[nodiscard]] bool isTrustedModulePath(const std::filesystem::path& module,
                                       const TrustedModuleRoots& roots);

/// Modules charges dont l'origine est `origin`.
[[nodiscard]] std::vector<std::filesystem::path>
loadedModulesWithOrigin(const TrustedModuleRoots& roots, ModuleOrigin origin);

/// Dossiers de confiance du processus courant.
[[nodiscard]] TrustedModuleRoots currentTrustedModuleRoots();

} // namespace enh::platform
