#pragma once

// Decodeur libavcodec (ADR-040) : NVDEC via hwaccel CUDA quand le magasin d'images fournit un
// contexte materiel et que selectDecoder() l'autorise ; sinon decodage logiciel, conversion en
// NV12 / P010 (libswscale) et journal « DecoderFallback ».

#include "enh/pipeline/core/decoder_selection.hpp"
#include "enh/pipeline/ffmpeg/av_raii.hpp"
#include "enh/pipeline/ffmpeg/frame_store.hpp"
#include "enh/pipeline/stage/decoder.hpp"
#include "enh/pipeline/stage/demuxer.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace enh::pipeline::ffmpeg {

struct FfmpegDecoderOptions {
    bool allow_hardware = true;
    int extra_hw_frames = 4;   // surfaces NVDEC en plus du minimum du codec
    int hardware_threads = 1;  // threads libavcodec quand NVDEC est tente
    int software_threads = 0;  // 0 = automatique
    std::string force_decoder; // tests : nom FFmpeg impose (vide = selectDecoder)
};

class FfmpegDecoder final : public IDecoder {
public:
    /// `par` et `time_base` viennent du demuxeur ; `store` recoit les images.
    [[nodiscard]] static Result<std::unique_ptr<FfmpegDecoder>>
    create(const AVCodecParameters& par, AVRational time_base, const VideoStreamInfo& info,
           std::shared_ptr<IFrameStore> store, const FfmpegDecoderOptions& options = {});

    Result<void> decode(const Packet* pkt, IFrameSink& out, std::stop_token st) override;
    [[nodiscard]] DecoderPath path() const noexcept override { return path_; }
    [[nodiscard]] std::uint64_t corruptPackets() const noexcept override { return corrupt_; }

    /// Choix initial (nom du decodeur, NVDEC tente, raison du repli).
    [[nodiscard]] const DecoderChoice& choice() const noexcept { return choice_; }
    [[nodiscard]] std::string_view decoderName() const noexcept;
    [[nodiscard]] const IFrameStore& store() const noexcept { return *store_; }

private:
    struct PrivateTag {};

public:
    FfmpegDecoder(PrivateTag /*tag*/, std::shared_ptr<IFrameStore> store,
                  const VideoStreamInfo& info) noexcept;
    FfmpegDecoder(const FfmpegDecoder&) = delete;
    FfmpegDecoder& operator=(const FfmpegDecoder&) = delete;
    FfmpegDecoder(FfmpegDecoder&&) = delete;
    FfmpegDecoder& operator=(FfmpegDecoder&&) = delete;
    ~FfmpegDecoder() override = default;

private:
    static AVPixelFormat getFormat(AVCodecContext* ctx, const AVPixelFormat* formats);
    Result<void> drain(IFrameSink& out, std::stop_token st, bool flushing);
    Result<void> deliver(AVFrame& frame, IFrameSink& out, const std::stop_token& st);
    Result<FrameBufferPtr> convertSoftware(const AVFrame& frame, const FrameDesc& desc,
                                           const std::stop_token& st);
    void noteSoftwareFrame();

    // Ordre de destruction (inverse) : contexte de codec, puis images de travail, puis magasin.
    std::shared_ptr<IFrameStore> store_;
    VideoStreamInfo info_;
    DecoderChoice choice_;
    FramePtr frame_;
    FramePtr sw_frame_;
    SwsContextPtr sws_;
    int sws_src_w_ = 0;
    int sws_src_h_ = 0;
    int sws_src_fmt_ = -1;
    int sws_dst_fmt_ = -1;
    CodecContextPtr ctx_;
    DecoderPath path_ = DecoderPath::Software;
    bool fallback_logged_ = false;
    bool hw_failed_ = false;
    std::uint64_t corrupt_ = 0;
    std::int64_t frames_ = 0;
};

} // namespace enh::pipeline::ffmpeg
