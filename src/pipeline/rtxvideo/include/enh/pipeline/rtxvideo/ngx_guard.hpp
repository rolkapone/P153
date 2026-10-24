#pragma once

// Garde-fous NGX constates a l'implementation d'ENH-040 (docs/gpu/ENH-040.md § 4) :
//
// 1. NGX (pilote 617.42) lance `nvngx_update.exe -api update ... -feature vsr -bootstrap` a chaque
//    initialisation, MEME avec l'identifiant d'application 0 et sans appel de mise a jour.
//    ChildProcessBlocker interdit la creation de tout processus enfant (Job Object,
//    JOB_OBJECT_LIMIT_ACTIVE_PROCESS = 1) pendant l'initialisation de NGX et la creation de
//    l'effet.
//
// 2. NGX charge ses modules sans le chargeur Windows (invisibles a EnumProcessModules) et peut
//    executer une copie de la DLL de la fonction rangee dans %ProgramData%\NVIDIA\NGX\models.
//    NgxLogObserver lit, dans le journal de NGX garde EN MEMOIRE (jamais ecrit), le chemin et la
//    version de la DLL de fonction reellement utilisee.

#include <cstdint>
#include <filesystem>
#include <mutex>
#include <string>
#include <string_view>

namespace enh::pipeline::rtxvideo {

/// RAII : pendant sa duree de vie, le processus ne peut creer aucun processus enfant. Le
/// processus est rattache une fois pour toutes a un Job Object propre a Enhance, dont la limite
/// est posee a la construction et levee a la destruction (imbrication autorisee, thread-safe).
/// Effet de bord assume : un autre thread de l'application qui lancerait un processus pendant
/// cette fenetre (~1 s) echouerait.
class ChildProcessBlocker {
public:
    ChildProcessBlocker() noexcept;
    ChildProcessBlocker(const ChildProcessBlocker&) = delete;
    ChildProcessBlocker& operator=(const ChildProcessBlocker&) = delete;
    ChildProcessBlocker(ChildProcessBlocker&&) = delete;
    ChildProcessBlocker& operator=(ChildProcessBlocker&&) = delete;
    ~ChildProcessBlocker();

    /// false si le Job Object n'a pas pu etre cree ou le processus rattache (blocage inactif).
    [[nodiscard]] bool active() const noexcept { return active_; }

    /// Nombre de processus enfants crees dans le job depuis son ouverture (0 attendu).
    [[nodiscard]] static std::uint32_t childProcessesCreated() noexcept;

private:
    bool active_ = false;
};

struct NgxSnippetInfo {
    std::filesystem::path path; // vide : NGX n'a pas annonce de DLL de fonction VSR
    std::string version;
};

/// Recoit les lignes du journal NGX (thread-safe) et retient la DLL de fonction VSR utilisee.
class NgxLogObserver {
public:
    /// Analyse une ligne. Format observe (pilote 617.42, niveau ON) :
    /// "[...] [NGXSecureLoadFeature:1499] app E658703 feature vsr snippet: <chemin> version: 1.6.0"
    void onLine(std::string_view line);

    [[nodiscard]] NgxSnippetInfo vsrSnippet() const;

private:
    mutable std::mutex mutex_;
    NgxSnippetInfo vsr_;
};

} // namespace enh::pipeline::rtxvideo
