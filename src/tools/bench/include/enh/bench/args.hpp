#pragma once

// Arguments de enhance-bench (docs/gpu/ENH-047.md § 2). Analyse ecrite a la main : fonction pure,
// sans dependance, testee en CI (tests/unit/bench).

#include "enh/expected.hpp"
#include "enh/pipeline/types.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace enh::bench {

enum class Stage : std::uint8_t { Decode, VsrImage };
enum class DecoderMode : std::uint8_t { Auto, Nvdec, Software };

/// Bornes des arguments numeriques (refus au-dela : code de sortie 1).
inline constexpr std::uint32_t kMaxRepeat = 1000;
inline constexpr std::uint32_t kMaxWarmup = 100'000;
inline constexpr std::uint32_t kMaxIterations = 100'000;
inline constexpr std::uint32_t kMaxSampleMs = 1000;
inline constexpr std::uint32_t kMaxOutputSidePx = 8192;

struct BenchArgs {
    bool help = false; // --help : afficher l'aide, code 0
    Stage stage = Stage::Decode;
    std::filesystem::path input;
    std::optional<std::filesystem::path> output; // vsr-image seulement (obligatoire)
    std::optional<std::filesystem::path> report; // absent : JSON sur la sortie standard
    bool force = false;                          // ecrase --report / --out existants
    std::uint32_t repeat = 1;
    std::uint32_t warmup = 30;
    std::uint64_t max_frames = 0; // 0 = tout le fichier (decode)
    bool allow_corrupt = false;   // decode : un decodage partiel reste mesure (code 0 + warnings)
    DecoderMode decoder = DecoderMode::Auto;
    std::uint8_t quality = 4; // NVSDK_NGX_VSR_QualityLevel 0..4 (vsr-image)
    std::uint8_t bits = 8;    // 8 ou 10 (vsr-image)
    pipeline::Size size{.width = 3840, .height = 2160};
    std::uint32_t iterations = 300;
    std::uint32_t sample_ms = 10;
};

/// Analyse argv[1..] (sans le nom du programme). Erreur : message en francais (code de sortie 1).
[[nodiscard]] enh::Expected<BenchArgs, std::string> parseArgs(std::span<const std::wstring> args);

/// Texte de --help (francais, UTF-8).
[[nodiscard]] std::string_view usageText() noexcept;

[[nodiscard]] std::string_view toString(Stage s) noexcept;
[[nodiscard]] std::string_view toString(DecoderMode d) noexcept;

/// Plus grand rectangle de rapport d'aspect `in` contenu dans `frame` (arrondi au pixel, >= 1).
[[nodiscard]] pipeline::Size fitInside(pipeline::Size in, pipeline::Size frame) noexcept;

} // namespace enh::bench
