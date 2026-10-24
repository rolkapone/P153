#pragma once

// Rapport JSON de enhance-bench, schema v1 (docs/gpu/ENH-047.md § 3, protocole QA § 4.3).
// Aucun chemin complet : seulement des noms de fichiers.

#include "enh/bench/args.hpp"
#include "enh/bench/exit_code.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace enh::bench {

inline constexpr int kSchemaVersion = 1;

struct InputInfo {
    std::string name; // nom du fichier seul
    std::uint64_t size_bytes = 0;
    std::string codec; // "h264", "bmp"...
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::string pix_fmt; // "nv12", "p010", "bgr24"...
};

struct StageTime {
    std::string name; // demux, decode, upload, vsr, download
    double ms_total = 0.0;
    std::uint64_t frames = 0; // diviseur de ms_per_frame (0 : ms_per_frame = ms_total)
};

struct RepeatResult {
    std::uint64_t frames_out = 0;
    double wall_s = 0.0;
    double vram_peak_mib = 0.0;  // processus
    double vram_after_mib = 0.0; // processus, apres destruction de l'etage
};

struct ErrorInfo {
    std::string code;    // PipelineErrorCode (nom stable) ou "InvalidArgs"
    std::string message; // francais, sans chemin
    std::string detail;  // technique (anglais), dossiers remplaces par <dir>
    std::int64_t native_code = 0;
};

struct BenchReport {
    // Contexte
    std::string tool_version;
    std::string git_sha;
    std::string gpu;
    std::string driver_version;
    int cuda_driver_api = 0;
    BenchArgs params;
    InputInfo input;
    std::string output_name;  // vsr-image : nom du BMP produit
    std::string decoder_path; // decode : "nvdec" ou "software" ; vide sinon
    std::string vsr_feature;  // vsr-image : origine et version de la DLL de fonction VSR

    // Images et temps (cumul sur les repetitions, chauffe exclue)
    std::uint64_t frames_in = 0;       // images decodees / appels VSR, chauffe comprise
    std::uint64_t frames_warmup = 0;   // dont chauffe (non chronometree)
    std::uint64_t frames_out = 0;      // images chronometrees
    std::uint64_t frames_per_pass = 0; // decode : images d'une passe (= ffprobe -count_frames)
    double wall_s = 0.0;               // duree murale chronometree
    std::vector<StageTime> stages;

    // VRAM (Mio)
    double vram_process_start_mib = 0.0; // avant toute initialisation CUDA
    double vram_context_mib = 0.0;       // apres creation du contexte CUDA
    double vram_rest_mib = 0.0;          // juste avant la creation de l'etage (1re repetition)
    double vram_peak_mib = 0.0;          // maximum pendant l'etage (toutes repetitions)
    double vram_io_mib = 0.0;            // vsr-image : images d'entree / sortie (hors etage)
    double vram_device_used_rest_mib = 0.0;
    double vram_device_used_peak_mib = 0.0;
    std::uint64_t vram_samples = 0;
    std::vector<RepeatResult> repeats;

    double private_start_mib = 0.0;
    double private_end_mib = 0.0;

    std::vector<std::string> warnings;
    ExitCode exit_code = ExitCode::Ok;
    std::optional<ErrorInfo> error;
};

/// fps_out = frames_out / wall_s (0 si wall_s == 0).
[[nodiscard]] double fpsOut(std::uint64_t frames, double wall_s) noexcept;

/// JSON du rapport (UTF-8, indente, ordre des champs stable).
[[nodiscard]] std::string toJsonText(const BenchReport& report);

} // namespace enh::bench
