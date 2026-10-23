#pragma once

// Journal du pipeline : codes et valeurs techniques seulement, jamais de chemin ni de metadonnees
// (SEC-14). L'application branche sa facade enh::log (ADR-007, ENH-025) avec setLogSink ; sans
// branchement, les lignes vont au debogueur (OutputDebugString).

#include <functional>
#include <string_view>

namespace enh::pipeline {

enum class LogLevel : unsigned char { Debug, Info, Warn, Error };

/// code : identifiant stable (ex. "DecoderFallback") ; message : detail technique en anglais.
using LogSink =
    std::function<void(LogLevel level, std::string_view code, std::string_view message)>;

/// Remplace le recepteur (thread-safe). Un recepteur vide retablit celui par defaut. Le recepteur
/// est appele hors verrou : un appel deja commence peut encore utiliser l ancien.
void setLogSink(LogSink sink);

/// Ecrit une ligne (thread-safe, noexcept : une erreur du recepteur est ignoree).
void logLine(LogLevel level, std::string_view code, std::string_view message) noexcept;

} // namespace enh::pipeline
