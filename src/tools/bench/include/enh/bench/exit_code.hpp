#pragma once

// Codes de sortie de enhance-bench (docs/gpu/ENH-047.md § 2), repris dans le champ exit_code du
// JSON.

#include "enh/pipeline/error.hpp"

#include <string_view>

namespace enh::bench {

enum class ExitCode : int {
    Ok = 0,
    InvalidArgs = 1,     // arguments invalides, sortie deja existante sans --force
    InputUnreadable = 2, // entree absente, illisible, corrompue ou format non pris en charge
    GpuUnavailable = 3,  // pas de carte, pilote trop ancien, composant (SDK, DLL) absent
    OutOfVram = 4,       // memoire video insuffisante
    InternalOrWrite = 5, // erreur interne, ou ecriture impossible (disque plein, acces refuse)
};

/// Code de sortie d'une erreur du pipeline.
[[nodiscard]] constexpr ExitCode exitCodeFor(pipeline::PipelineErrorCode code) noexcept {
    using pipeline::PipelineErrorCode;
    switch (code) {
    case PipelineErrorCode::InvalidParams:
    case PipelineErrorCode::OutputExists:
        return ExitCode::InvalidArgs;
    case PipelineErrorCode::SourceCorrupt:
    case PipelineErrorCode::UnsupportedFormat:
    case PipelineErrorCode::SourceMissing:
    case PipelineErrorCode::SourceUnreadable:
        return ExitCode::InputUnreadable;
    case PipelineErrorCode::ComponentMissing:
    case PipelineErrorCode::DeviceLost:
    case PipelineErrorCode::Busy:
        return ExitCode::GpuUnavailable;
    case PipelineErrorCode::OutOfVram:
        return ExitCode::OutOfVram;
    case PipelineErrorCode::DiskFull:
    case PipelineErrorCode::WriteDenied:
    case PipelineErrorCode::Internal:
    case PipelineErrorCode::Cancelled:
        return ExitCode::InternalOrWrite;
    }
    return ExitCode::InternalOrWrite;
}

[[nodiscard]] constexpr int toInt(ExitCode c) noexcept {
    return static_cast<int>(c);
}

/// Message francais, court et sans chemin, pour une erreur du pipeline (console et JSON).
[[nodiscard]] std::string_view frenchMessage(pipeline::PipelineErrorCode code) noexcept;

} // namespace enh::bench
