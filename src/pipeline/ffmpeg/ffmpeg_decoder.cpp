#include "enh/pipeline/ffmpeg/ffmpeg_decoder.hpp"

#include "enh/pipeline/core/frame_layout.hpp"
#include "enh/pipeline/core/log.hpp"
#include "enh/pipeline/ffmpeg/av_error.hpp"
#include "enh/pipeline/ffmpeg/av_mapping.hpp"

extern "C" {
#include <libavutil/hwcontext.h>
#include <libavutil/imgutils.h>
#include <libavutil/pixdesc.h>
}

#include <array>
#include <cerrno>
#include <climits>
#include <cstring>
#include <utility>

namespace enh::pipeline::ffmpeg {

namespace {

Result<void> cancelledResult() {
    return fail(PipelineErrorCode::Cancelled, "decode cancelled");
}

bool isHardwareFormat(AVPixelFormat f) noexcept {
    const AVPixFmtDescriptor* d = av_pix_fmt_desc_get(f);
    return d != nullptr && (d->flags & AV_PIX_FMT_FLAG_HWACCEL) != 0;
}

} // namespace

FfmpegDecoder::FfmpegDecoder(PrivateTag /*tag*/, std::shared_ptr<IFrameStore> store,
                             VideoStreamInfo info)
    : store_(std::move(store)), info_(std::move(info)) {}

std::string_view FfmpegDecoder::decoderName() const noexcept {
    return (ctx_ && ctx_->codec != nullptr) ? std::string_view{ctx_->codec->name}
                                            : std::string_view{};
}

AVPixelFormat FfmpegDecoder::getFormat(AVCodecContext* ctx, const AVPixelFormat* formats) noexcept {
    auto* self = static_cast<FfmpegDecoder*>(ctx->opaque);
    bool cuda_offered = false;
    AVPixelFormat first_software = AV_PIX_FMT_NONE;
    for (const AVPixelFormat* p = formats; *p != AV_PIX_FMT_NONE; ++p) {
        if (*p == AV_PIX_FMT_CUDA) {
            cuda_offered = true;
        } else if (first_software == AV_PIX_FMT_NONE && !isHardwareFormat(*p)) {
            first_software = *p;
        }
    }
    if (self->choice_.try_hardware && cuda_offered && !self->hw_failed_) {
        return AV_PIX_FMT_CUDA;
    }
    if (self->choice_.try_hardware) {
        // Rappel sans CUDA alors que NVDEC etait tente : l'initialisation du hwaccel a echoue
        // (profil non gere...). FFmpeg continue en logiciel ; on ne retente pas.
        self->hw_failed_ = true;
    }
    return first_software;
}

Result<std::unique_ptr<FfmpegDecoder>> FfmpegDecoder::create(const AVCodecParameters& par,
                                                             AVRational time_base,
                                                             const VideoStreamInfo& info,
                                                             std::shared_ptr<IFrameStore> store,
                                                             const FfmpegDecoderOptions& options) {
    if (!store) {
        return fail(PipelineErrorCode::InvalidParams, "frame store is null");
    }
    installFfmpegLogBridge();
    const bool hw_available = options.allow_hardware && store->hwDevice() != nullptr;
    auto self = std::make_unique<FfmpegDecoder>(PrivateTag{}, std::move(store), info);
    self->choice_ = selectDecoder(info, hw_available);

    const std::string name = options.force_decoder.empty() ? std::string(self->choice_.decoder_name)
                                                           : options.force_decoder;
    const AVCodec* codec = name.empty() ? nullptr : avcodec_find_decoder_by_name(name.c_str());
    if (codec == nullptr) {
        codec = avcodec_find_decoder(par.codec_id); // decodeur par defaut de la build
    }
    if (codec == nullptr || codec->id != par.codec_id) {
        return fail(PipelineErrorCode::UnsupportedFormat,
                    std::string("no decoder for codec ") + avcodec_get_name(par.codec_id));
    }

    self->ctx_.reset(avcodec_alloc_context3(codec));
    AVCodecContext* ctx = self->ctx_.get();
    if (ctx == nullptr) {
        return fail(PipelineErrorCode::Internal, "avcodec_alloc_context3 failed");
    }
    if (const int r = avcodec_parameters_to_context(ctx, &par); r < 0) {
        return enh::Unexpected<PipelineError>(
            avError(PipelineErrorCode::Internal, "avcodec_parameters_to_context", r));
    }
    ctx->pkt_timebase = time_base;
    ctx->opaque = self.get();
    ctx->get_format = &FfmpegDecoder::getFormat;
    if (self->choice_.try_hardware) {
        ctx->hw_device_ctx = av_buffer_ref(self->store_->hwDevice());
        if (ctx->hw_device_ctx == nullptr) {
            return fail(PipelineErrorCode::Internal, "av_buffer_ref(hw device) failed");
        }
        ctx->extra_hw_frames = options.extra_hw_frames;
        ctx->thread_count = options.hardware_threads;
    } else {
        ctx->thread_count = options.software_threads;
    }
    if (const int r = avcodec_open2(ctx, codec, nullptr); r < 0) {
        return enh::Unexpected<PipelineError>(
            avError(PipelineErrorCode::UnsupportedFormat, "avcodec_open2", r));
    }
    self->frame_.reset(av_frame_alloc());
    self->sw_frame_.reset(av_frame_alloc());
    if (!self->frame_ || !self->sw_frame_) {
        return fail(PipelineErrorCode::Internal, "av_frame_alloc failed");
    }
    self->path_ = self->choice_.try_hardware ? DecoderPath::Nvdec : DecoderPath::Software;
    if (!self->choice_.try_hardware) {
        const bool expected = self->choice_.reason == "no-nvidia-gpu";
        std::string msg = "codec=" + info.codec_name + " decoder=" + codec->name +
                          " reason=" + std::string(self->choice_.reason);
        logLine(expected ? LogLevel::Info : LogLevel::Warn, "DecoderFallback", msg);
        self->fallback_logged_ = true;
    }
    return self;
}

Result<void> FfmpegDecoder::decode(const Packet* pkt, IFrameSink& out, std::stop_token st) {
    if (st.stop_requested()) {
        return cancelledResult();
    }
    AVPacket* avpkt = nullptr;
    PacketPtr temp;
    if (pkt != nullptr) {
        if (pkt->payload && pkt->payload->native() != nullptr) {
            avpkt = static_cast<AVPacket*>(pkt->payload->native());
        } else {
            const auto bytes = pkt->payload ? pkt->payload->bytes() : std::span<const std::byte>{};
            if (bytes.size() > static_cast<std::size_t>(INT_MAX - AV_INPUT_BUFFER_PADDING_SIZE)) {
                return fail(PipelineErrorCode::SourceCorrupt, "packet too large");
            }
            temp.reset(av_packet_alloc());
            if (!temp) {
                return fail(PipelineErrorCode::Internal, "av_packet_alloc failed");
            }
            if (const int r = av_new_packet(temp.get(), static_cast<int>(bytes.size())); r < 0) {
                return enh::Unexpected<PipelineError>(
                    avError(PipelineErrorCode::Internal, "av_new_packet", r));
            }
            if (!bytes.empty()) {
                std::memcpy(temp->data, bytes.data(), bytes.size());
            }
            temp->pts = pkt->pts == kNoPts ? AV_NOPTS_VALUE : pkt->pts;
            temp->dts = pkt->dts == kNoPts ? AV_NOPTS_VALUE : pkt->dts;
            temp->duration = pkt->duration;
            temp->flags = pkt->keyframe ? AV_PKT_FLAG_KEY : 0;
            avpkt = temp.get();
        }
        if (avpkt->size <= 0) {
            return {}; // un paquet vide signifierait « vidange » a avcodec_send_packet
        }
        if (pkt->corrupt) {
            ++corrupt_;
        }
    }

    for (;;) {
        const int r = avcodec_send_packet(ctx_.get(), avpkt);
        if (r == AVERROR(EAGAIN)) {
            // Sortie pleine : vider puis renvoyer le meme paquet.
            if (auto d = drain(out, st, false); !d) {
                return d;
            }
            continue;
        }
        if (r == AVERROR_INVALIDDATA) {
            if (pkt == nullptr || !pkt->corrupt) {
                ++corrupt_; // paquet illisible : image masquee, le flux continue (deja compte
            } // s il etait marque corrompu par le demuxeur)
            break;
        }
        if (r < 0 && r != AVERROR_EOF) {
            return enh::Unexpected<PipelineError>(
                avError(PipelineErrorCode::SourceCorrupt, "avcodec_send_packet", r));
        }
        break;
    }
    return drain(out, st, pkt == nullptr);
}

Result<void> FfmpegDecoder::drain(IFrameSink& out, std::stop_token st, bool flushing) {
    for (;;) {
        if (st.stop_requested()) {
            return cancelledResult();
        }
        const int r = avcodec_receive_frame(ctx_.get(), frame_.get());
        if (r == AVERROR(EAGAIN) || r == AVERROR_EOF) {
            (void)flushing; // en vidange, EOF termine ; EAGAIN n'arrive pas
            return {};
        }
        if (r == AVERROR_INVALIDDATA) {
            ++corrupt_;
            continue;
        }
        if (r < 0) {
            return enh::Unexpected<PipelineError>(avError(path_ == DecoderPath::Nvdec
                                                              ? PipelineErrorCode::Internal
                                                              : PipelineErrorCode::SourceCorrupt,
                                                          "avcodec_receive_frame", r));
        }
        auto delivered = deliver(*frame_, out, st);
        av_frame_unref(frame_.get());
        av_frame_unref(sw_frame_.get());
        if (!delivered) {
            return delivered;
        }
    }
}

void FfmpegDecoder::noteSoftwareFrame() {
    path_ = DecoderPath::Software;
    if (!fallback_logged_) {
        fallback_logged_ = true;
        logLine(LogLevel::Warn, "DecoderFallback",
                "codec=" + info_.codec_name + " decoder=" + std::string(decoderName()) +
                    " reason=nvdec-init-failed");
    }
}

Result<void> FfmpegDecoder::deliver(AVFrame& frame, IFrameSink& out, const std::stop_token& st) {
    const Size size{static_cast<std::uint32_t>(frame.width > 0 ? frame.width : 0),
                    static_cast<std::uint32_t>(frame.height > 0 ? frame.height : 0)};
    if (auto dims = checkDimensions(size); !dims) {
        return dims;
    }
    FrameDesc desc;
    desc.size_px = size;
    desc.color =
        toColorInfo(frame.colorspace, frame.color_primaries, frame.color_trc, frame.color_range);

    Result<FrameBufferPtr> buffer = fail(PipelineErrorCode::Internal, "no frame produced");
    const AVFrame* software = &frame;
    bool on_device = false;
    if (frame.format == AV_PIX_FMT_CUDA && frame.hw_frames_ctx != nullptr) {
        const auto* frames_ctx =
            reinterpret_cast<const AVHWFramesContext*>(frame.hw_frames_ctx->data);
        const AVPixelFormat sw = frames_ctx->sw_format;
        if (sw == AV_PIX_FMT_NV12 || sw == AV_PIX_FMT_P010LE || sw == AV_PIX_FMT_P016LE) {
            desc.format = sw == AV_PIX_FMT_NV12 ? PixelFormat::Nv12 : PixelFormat::P010;
            buffer = store_->importDevice(frame, desc, st);
            on_device = true;
            path_ = DecoderPath::Nvdec;
        } else {
            // Format de surface inattendu (4:4:4...) : rapatriement puis chemin logiciel.
            if (const int r = av_hwframe_transfer_data(sw_frame_.get(), &frame, 0); r < 0) {
                return enh::Unexpected<PipelineError>(
                    avError(PipelineErrorCode::Internal, "av_hwframe_transfer_data", r));
            }
            software = sw_frame_.get();
        }
    }
    if (!on_device) {
        const PixelFormatTraits traits =
            pixelFormatTraits(static_cast<AVPixelFormat>(software->format));
        if (!traits.known) {
            return fail(PipelineErrorCode::UnsupportedFormat,
                        std::string("unsupported decoded pixel format ") +
                            (av_get_pix_fmt_name(static_cast<AVPixelFormat>(software->format))
                                 ? av_get_pix_fmt_name(static_cast<AVPixelFormat>(software->format))
                                 : "none"));
        }
        desc.format = traits.bit_depth > 8 ? PixelFormat::P010 : PixelFormat::Nv12;
        const SoftwareColorPlan color = planSoftwareColor(
            static_cast<AVPixelFormat>(software->format), software->colorspace, desc.color);
        desc.color = color.output; // etiquette = ce que contiennent vraiment les pixels
        buffer = convertSoftware(*software, desc, color, st);
        if (buffer) {
            noteSoftwareFrame();
        }
    }
    if (!buffer) {
        return enh::Unexpected<PipelineError>(std::move(buffer.error()));
    }

    VideoFrame vf;
    vf.desc = desc;
    vf.pts = frame.best_effort_timestamp == AV_NOPTS_VALUE ? kNoPts : frame.best_effort_timestamp;
    vf.time_base = info_.time_base;
    vf.index = frames_++;
    vf.buffer = std::move(*buffer);
    return out.push(std::move(vf), st);
}

Result<FrameBufferPtr> FfmpegDecoder::convertSoftware(const AVFrame& frame, const FrameDesc& desc,
                                                      const SoftwareColorPlan& color,
                                                      const std::stop_token& st) {
    auto staging = store_->acquireStaging(desc, st);
    if (!staging) {
        return staging;
    }
    const auto planes = (*staging)->planes();
    if (planes.size() < 2) {
        return fail(PipelineErrorCode::Internal, "staging buffer has fewer than 2 planes");
    }
    std::array<std::uint8_t*, 4> dst_data{};
    std::array<int, 4> dst_linesize{};
    for (std::size_t i = 0; i < 2; ++i) {
        if (planes[i].pitch_bytes > static_cast<std::size_t>(INT_MAX) ||
            planes[i].width_bytes > static_cast<std::size_t>(INT_MAX) ||
            planes[i].rows > static_cast<std::size_t>(INT_MAX)) {
            return fail(PipelineErrorCode::UnsupportedFormat, "plane too large");
        }
        dst_data[i] = reinterpret_cast<std::uint8_t*>(planes[i].address);
        dst_linesize[i] = static_cast<int>(planes[i].pitch_bytes);
    }
    const AVPixelFormat dst_fmt =
        desc.format == PixelFormat::P010 ? AV_PIX_FMT_P010LE : AV_PIX_FMT_NV12;
    if (frame.format == dst_fmt) {
        for (std::size_t i = 0; i < 2; ++i) {
            av_image_copy_plane(dst_data[i], dst_linesize[i], frame.data[i], frame.linesize[i],
                                static_cast<int>(planes[i].width_bytes),
                                static_cast<int>(planes[i].rows));
        }
    } else {
        if (!sws_ || sws_src_w_ != frame.width || sws_src_h_ != frame.height ||
            sws_src_fmt_ != frame.format || sws_dst_fmt_ != dst_fmt || sws_color_ != color) {
            sws_.reset(sws_getContext(
                frame.width, frame.height, static_cast<AVPixelFormat>(frame.format), frame.width,
                frame.height, dst_fmt, SWS_BILINEAR, nullptr, nullptr, nullptr));
            if (!sws_) {
                return fail(PipelineErrorCode::UnsupportedFormat,
                            "sws_getContext failed for this pixel format");
            }
            // Plage et matrice explicites : pas de conversion implicite yuvj -> plage limitee,
            // RVB -> YUV en BT.709 (ADR-042).
            if (sws_setColorspaceDetails(sws_.get(), sws_getCoefficients(color.src_colorspace),
                                         color.src_full_range ? 1 : 0,
                                         sws_getCoefficients(color.dst_colorspace),
                                         color.dst_full_range ? 1 : 0, 0, 1 << 16, 1 << 16) < 0) {
                return fail(PipelineErrorCode::UnsupportedFormat,
                            "sws_setColorspaceDetails failed");
            }
            sws_color_ = color;
            sws_src_w_ = frame.width;
            sws_src_h_ = frame.height;
            sws_src_fmt_ = frame.format;
            sws_dst_fmt_ = dst_fmt;
        }
        const int r = sws_scale(sws_.get(), frame.data, frame.linesize, 0, frame.height,
                                dst_data.data(), dst_linesize.data());
        if (r < 0) {
            return enh::Unexpected<PipelineError>(
                avError(PipelineErrorCode::Internal, "sws_scale", r));
        }
    }
    return store_->commit(std::move(*staging), st);
}

} // namespace enh::pipeline::ffmpeg
