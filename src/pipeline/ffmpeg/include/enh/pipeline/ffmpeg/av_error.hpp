#pragma once

// Conversion des codes retour FFmpeg (int < 0) en PipelineError (ADR-047).

#include "enh/pipeline/error.hpp"

#include <source_location>
#include <string>
#include <string_view>

namespace enh::pipeline::ffmpeg {

/// Texte de av_strerror (anglais).
[[nodiscard]] std::string avErrorText(int averror);

/// Erreur FFmpeg : "<what>: <av_strerror> (<code>)", code natif = AVERROR.
[[nodiscard]] PipelineError avError(PipelineErrorCode code, std::string_view what, int averror,
                                    std::source_location where = std::source_location::current());

/// Redirige av_log vers le journal du pipeline (une seule fois par processus, niveau
/// avertissement et plus ; les lignes de FFmpeg ne contiennent pas de chemin : aucun chemin texte
/// ne lui est donne, ADR-049).
void installFfmpegLogBridge() noexcept;

} // namespace enh::pipeline::ffmpeg
