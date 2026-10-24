#include "enh/bench/exit_code.hpp"

namespace enh::bench {

std::string_view frenchMessage(pipeline::PipelineErrorCode code) noexcept {
    using pipeline::PipelineErrorCode;
    switch (code) {
    case PipelineErrorCode::SourceCorrupt:
        return "Le fichier d'entree est abime ou incoherent.";
    case PipelineErrorCode::UnsupportedFormat:
        return "Le format du fichier d'entree n'est pas pris en charge.";
    case PipelineErrorCode::OutOfVram:
        return "Memoire video insuffisante pour cet etage.";
    case PipelineErrorCode::DiskFull:
        return "Disque plein : le fichier de sortie n'a pas pu etre ecrit.";
    case PipelineErrorCode::DeviceLost:
        return "La carte graphique a cesse de repondre.";
    case PipelineErrorCode::WriteDenied:
        return "Ecriture impossible dans le dossier de sortie.";
    case PipelineErrorCode::SourceMissing:
        return "Le fichier d'entree est introuvable.";
    case PipelineErrorCode::Internal:
        return "Erreur interne du banc.";
    case PipelineErrorCode::Cancelled:
        return "Mesure interrompue.";
    case PipelineErrorCode::SourceUnreadable:
        return "Le fichier d'entree ne peut pas etre lu.";
    case PipelineErrorCode::ComponentMissing:
        return "Carte NVIDIA, pilote ou composant RTX Video absent ou trop ancien.";
    case PipelineErrorCode::OutputExists:
        return "Le fichier de sortie existe deja (ajouter --force pour l'ecraser).";
    case PipelineErrorCode::InvalidParams:
        return "Parametres invalides pour cette entree.";
    case PipelineErrorCode::Busy:
        return "Le GPU ou le runtime RTX Video est deja utilise par ce processus.";
    }
    return "Erreur inconnue.";
}

} // namespace enh::bench
