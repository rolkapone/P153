#pragma once

// Erreurs du pipeline (ADR-047, docs/gpu/architecture-pipeline.md § 6.6) : codes calques sur P-08,
// aucune phrase destinee a l'utilisateur (la presentation traduit le code).

#include "enh/expected.hpp"

#include <chrono>
#include <cstdint>
#include <optional>
#include <source_location>
#include <string>
#include <string_view>
#include <utility>

namespace enh::pipeline {

enum class PipelineErrorCode : std::uint16_t {
    // Une ligne du tableau P-08 = un code
    SourceCorrupt,
    UnsupportedFormat,
    OutOfVram,
    DiskFull,
    DeviceLost,
    WriteDenied,
    SourceMissing,
    Internal,
    // Hors P-08
    Cancelled,
    SourceUnreadable,
    ComponentMissing,
    OutputExists,
    InvalidParams,
    Busy,
};

enum class Component : std::uint8_t {
    None,
    NvidiaGpu,
    Driver,
    NvencHevc,
    NvencAv1,
    Nvenc10Bit,
    RtxVideoSdk,
    TensorRt,
    InterpolationModel,
};

struct PipelineError {
    PipelineErrorCode code = PipelineErrorCode::Internal;
    std::optional<std::chrono::duration<double>> at; // SourceCorrupt : instant dans la source (s)
    std::uint32_t required_mib = 0;                  // OutOfVram
    std::uint32_t available_mib = 0;                 // OutOfVram
    Component component = Component::None;           // ComponentMissing
    std::string detail;           // anglais, technique : journal et « Copier le detail »
    std::int64_t native_code = 0; // CUresult, cudaError_t, AVERROR, GetLastError...
};

/// Raccourci local (ADR-047) : meme type que enh::Expected<T, PipelineError>.
template <typename T> using Result = enh::Expected<T, PipelineError>;

namespace detail {
/// Garde la partie du chemin source a partir de "src/" (pas de chemin de build : SEC-16).
constexpr std::string_view trimSourcePath(std::string_view file) noexcept {
    for (const std::string_view marker : {"src/", "src\\", "tests/", "tests\\"}) {
        const auto pos = file.rfind(marker);
        if (pos != std::string_view::npos) {
            return file.substr(pos);
        }
    }
    const auto slash = file.find_last_of("/\\");
    return slash == std::string_view::npos ? file : file.substr(slash + 1);
}
} // namespace detail

/// Construit une erreur avec son emplacement dans le code (pour le journal).
[[nodiscard]] inline PipelineError
makeError(PipelineErrorCode code, std::string detail, std::int64_t native_code = 0,
          std::source_location where = std::source_location::current()) {
    PipelineError e;
    e.code = code;
    e.native_code = native_code;
    e.detail = std::move(detail);
    e.detail += " [";
    e.detail += detail::trimSourcePath(where.file_name());
    e.detail += ':';
    e.detail += std::to_string(where.line());
    e.detail += ']';
    return e;
}

/// Erreur prete a etre renvoyee par une fonction qui retourne Result<T>.
[[nodiscard]] inline enh::Unexpected<PipelineError>
fail(PipelineErrorCode code, std::string detail, std::int64_t native_code = 0,
     std::source_location where = std::source_location::current()) {
    return enh::Unexpected<PipelineError>(makeError(code, std::move(detail), native_code, where));
}

/// Nom stable d'un code (journal, tests). Jamais affiche tel quel a l'utilisateur.
constexpr std::string_view toString(PipelineErrorCode code) noexcept {
    switch (code) {
    case PipelineErrorCode::SourceCorrupt:
        return "SourceCorrupt";
    case PipelineErrorCode::UnsupportedFormat:
        return "UnsupportedFormat";
    case PipelineErrorCode::OutOfVram:
        return "OutOfVram";
    case PipelineErrorCode::DiskFull:
        return "DiskFull";
    case PipelineErrorCode::DeviceLost:
        return "DeviceLost";
    case PipelineErrorCode::WriteDenied:
        return "WriteDenied";
    case PipelineErrorCode::SourceMissing:
        return "SourceMissing";
    case PipelineErrorCode::Internal:
        return "Internal";
    case PipelineErrorCode::Cancelled:
        return "Cancelled";
    case PipelineErrorCode::SourceUnreadable:
        return "SourceUnreadable";
    case PipelineErrorCode::ComponentMissing:
        return "ComponentMissing";
    case PipelineErrorCode::OutputExists:
        return "OutputExists";
    case PipelineErrorCode::InvalidParams:
        return "InvalidParams";
    case PipelineErrorCode::Busy:
        return "Busy";
    }
    return "Unknown";
}

} // namespace enh::pipeline
