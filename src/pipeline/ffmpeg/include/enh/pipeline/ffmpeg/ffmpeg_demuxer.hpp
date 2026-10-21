#pragma once

// Demuxeur libavformat (ADR-049) : AVIOContext maison, ouvertures secondaires refusees (io_open),
// protocol_whitelist=file, dimensions bornees avant toute allocation (SEC-13).

#include "enh/pipeline/ffmpeg/av_raii.hpp"
#include "enh/pipeline/ffmpeg/avio_reader.hpp"
#include "enh/pipeline/stage/demuxer.hpp"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string_view>

namespace enh::pipeline::ffmpeg {

/// Correspondance AVCodecID -> SourceCodec (fonction pure).
[[nodiscard]] SourceCodec toSourceCodec(AVCodecID id) noexcept;

/// Demuxeurs refuses : ils ouvrent d'autres fichiers sans passer par io_open (concat, hls, dash,
/// imf, image2...) ou des flux reseau (sdp, rtp, rtsp). Fonction pure.
[[nodiscard]] bool isRefusedDemuxer(std::string_view name) noexcept;

class FfmpegDemuxer final : public IDemuxer {
public:
    [[nodiscard]] static Result<std::unique_ptr<FfmpegDemuxer>>
    open(const std::filesystem::path& path);

    [[nodiscard]] const VideoStreamInfo& videoStream() const noexcept override { return info_; }
    Result<std::optional<Packet>> read() override;
    Result<void> seek(std::chrono::duration<double> t) override;

    /// Parametres du codec de la piste video (pour creer le decodeur).
    [[nodiscard]] const AVCodecParameters& codecParameters() const noexcept;
    [[nodiscard]] AVRational streamTimeBase() const noexcept;
    /// Nombre d'ouvertures secondaires refusees depuis le debut du processus (HLS, concat,
    /// references externes...). Compteur global : le rappel ne dereference pas s->opaque, que des
    /// demuxeurs imbriques de FFmpeg peuvent utiliser pour eux-memes.
    [[nodiscard]] static std::int64_t refusedSecondaryOpensTotal() noexcept;

private:
    struct PrivateTag {};

public:
    explicit FfmpegDemuxer(PrivateTag /*tag*/) noexcept {}
    FfmpegDemuxer(const FfmpegDemuxer&) = delete;
    FfmpegDemuxer& operator=(const FfmpegDemuxer&) = delete;
    FfmpegDemuxer(FfmpegDemuxer&&) = delete;
    FfmpegDemuxer& operator=(FfmpegDemuxer&&) = delete;
    ~FfmpegDemuxer() override = default;

private:
    static int refuseIoOpen(AVFormatContext* s, AVIOContext** pb, const char* url, int flags,
                            AVDictionary** options);
    [[nodiscard]] PipelineError readError(int averror) const;

    // Ordre des membres = ordre inverse de destruction : le contexte de format (fmt_) est ferme
    // AVANT l'AVIO (io_), lui-meme ferme avant le fichier qu'il possede.
    std::unique_ptr<AvioReader> io_;
    FormatInputPtr fmt_;
    VideoStreamInfo info_;
    std::int64_t last_pts_ = kNoPts;
};

} // namespace enh::pipeline::ffmpeg
