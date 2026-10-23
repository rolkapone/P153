#pragma once

// Demuxeur (interface INTERNE au pipeline, docs/gpu/architecture-pipeline.md § 3.2).

#include "enh/pipeline/stage/packet.hpp"

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>

namespace enh::pipeline {

/// Codecs video reconnus par le pipeline (correspondance avec les codecs FFmpeg dans
/// src/pipeline/ffmpeg ; logique de choix pure dans decoder_selection.hpp).
enum class SourceCodec : std::uint8_t {
    H264,
    Hevc,
    Av1,
    Vp9,
    Vp8,
    Mpeg2,
    Mpeg1,
    Mpeg4,
    Vc1,
    ProRes,
    Ffv1,
    Other,
};

enum class ChromaSubsampling : std::uint8_t { Yuv420, Yuv422, Yuv444, Gray, Other };

/// Ce que le decodeur doit savoir de la piste video choisie.
struct VideoStreamInfo {
    int stream_index = -1;
    SourceCodec codec = SourceCodec::Other;
    std::string codec_name; // nom FFmpeg du codec (journal)
    int profile = -1;       // profil du flux (valeur FFmpeg), -1 si inconnu
    Size coded_size_px;
    int bit_depth = 8;
    ChromaSubsampling chroma = ChromaSubsampling::Yuv420;
    Rational time_base;
    Rational avg_fps;
    std::chrono::duration<double> duration{0.0};
    std::int64_t frame_count_estimate = 0; // 0 si inconnu
    ColorInfo color;
};

class IDemuxer {
public:
    IDemuxer() = default;
    IDemuxer(const IDemuxer&) = delete;
    IDemuxer& operator=(const IDemuxer&) = delete;
    IDemuxer(IDemuxer&&) = delete;
    IDemuxer& operator=(IDemuxer&&) = delete;
    virtual ~IDemuxer() = default;

    [[nodiscard]] virtual const VideoStreamInfo& videoStream() const noexcept = 0;
    /// Paquet suivant de la piste video ; nullopt = fin du fichier.
    virtual Result<std::optional<Packet>> read() = 0;
    /// Recherche a l'image cle precedant t (apercu, reprise).
    virtual Result<void> seek(std::chrono::duration<double> t) = 0;
};

} // namespace enh::pipeline
