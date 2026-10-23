#include "enh/pipeline/ffmpeg/ffmpeg_demuxer.hpp"

#include "enh/pipeline/core/frame_layout.hpp"
#include "enh/pipeline/core/log.hpp"
#include "enh/pipeline/ffmpeg/av_error.hpp"
#include "enh/pipeline/ffmpeg/av_mapping.hpp"

extern "C" {
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
}

#include <array>
#include <atomic>
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <limits>
#include <span>
#include <utility>

namespace enh::pipeline::ffmpeg {

namespace {

std::atomic<std::int64_t> g_refused_opens{0};

/// Paquet FFmpeg porte tel quel (aucune copie des octets).
class FfmpegPacketPayload final : public PacketPayload {
public:
    explicit FfmpegPacketPayload(PacketPtr pkt) noexcept : pkt_(std::move(pkt)) {}
    [[nodiscard]] std::span<const std::byte> bytes() const noexcept override {
        return {reinterpret_cast<const std::byte*>(pkt_->data),
                static_cast<std::size_t>(pkt_->size > 0 ? pkt_->size : 0)};
    }
    [[nodiscard]] void* native() noexcept override { return pkt_.get(); }

private:
    PacketPtr pkt_;
};

bool isUsableVideo(const AVStream* st) noexcept {
    return st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO &&
           (st->disposition & AV_DISPOSITION_ATTACHED_PIC) == 0;
}

/// SEC-13 : bornes 1..8192 et av_image_check_size2, avant toute allocation de notre part.
Result<void> checkStreamDimensions(const AVCodecParameters& par) {
    const Size size{static_cast<std::uint32_t>(par.width > 0 ? par.width : 0),
                    static_cast<std::uint32_t>(par.height > 0 ? par.height : 0)};
    if (auto r = checkDimensions(size); !r) {
        return r;
    }
    const int r =
        av_image_check_size2(static_cast<unsigned>(par.width), static_cast<unsigned>(par.height),
                             std::numeric_limits<std::int64_t>::max(),
                             static_cast<AVPixelFormat>(par.format), 0, nullptr);
    if (r < 0) {
        return enh::Unexpected<PipelineError>(
            avError(PipelineErrorCode::UnsupportedFormat, "av_image_check_size2", r));
    }
    return {};
}

} // namespace

namespace {

/// Vrai si l'un des noms de `name` (liste FFmpeg separee par des virgules, ex.
/// "mov,mp4,m4a,3gp,3g2,mj2") figure dans `list`.
template <std::size_t N>
bool anyNameIn(std::string_view name, const std::array<std::string_view, N>& list) noexcept {
    std::size_t start = 0;
    while (start <= name.size()) {
        const std::size_t comma = name.find(',', start);
        const std::string_view part = name.substr(
            start, comma == std::string_view::npos ? std::string_view::npos : comma - start);
        for (const std::string_view entry : list) {
            if (!part.empty() && part == entry) {
                return true;
            }
        }
        if (comma == std::string_view::npos) {
            break;
        }
        start = comma + 1;
    }
    return false;
}

} // namespace

bool isRefusedDemuxer(std::string_view name) noexcept {
    // Second filet : demuxeurs qui lisent d'autres fichiers (listes, sous-titres VobSub, scripts
    // AviSynth / VapourSynth, DVD) ou des flux reseau (sdp, rtp, rtsp, sap).
    constexpr std::array<std::string_view, 15> kRefused = {
        "concat", "hls",        "dash", "imf",    "sdp",      "rtp",         "rtsp",    "sap",
        "image2", "ffmetadata", "tee",  "vobsub", "avisynth", "vapoursynth", "dvdvideo"};
    return anyNameIn(name, kRefused);
}

bool isAllowedDemuxer(std::string_view name) noexcept {
    // Liste blanche (ADR-049, revue ENH-008 C1) : conteneurs de la matrice ENH-008 / ENH-008b et
    // flux video bruts courants. Tout autre demuxeur est refuse (UnsupportedFormat).
    constexpr std::array<std::string_view, 14> kAllowed = {
        "mov", "matroska", "mpegts", "avi",  "mpeg", "asf",       "flv",
        "mxf", "ivf",      "obu",    "h264", "hevc", "mpegvideo", "m4v"};
    return !isRefusedDemuxer(name) && anyNameIn(name, kAllowed);
}

SourceCodec toSourceCodec(AVCodecID id) noexcept {
    switch (id) {
    case AV_CODEC_ID_H264:
        return SourceCodec::H264;
    case AV_CODEC_ID_HEVC:
        return SourceCodec::Hevc;
    case AV_CODEC_ID_AV1:
        return SourceCodec::Av1;
    case AV_CODEC_ID_VP9:
        return SourceCodec::Vp9;
    case AV_CODEC_ID_VP8:
        return SourceCodec::Vp8;
    case AV_CODEC_ID_MPEG2VIDEO:
        return SourceCodec::Mpeg2;
    case AV_CODEC_ID_MPEG1VIDEO:
        return SourceCodec::Mpeg1;
    case AV_CODEC_ID_MPEG4:
        return SourceCodec::Mpeg4;
    case AV_CODEC_ID_VC1:
    case AV_CODEC_ID_WMV3:
        return SourceCodec::Vc1;
    case AV_CODEC_ID_PRORES:
        return SourceCodec::ProRes;
    case AV_CODEC_ID_FFV1:
        return SourceCodec::Ffv1;
    default:
        return SourceCodec::Other;
    }
}

std::int64_t FfmpegDemuxer::refusedSecondaryOpensTotal() noexcept {
    return g_refused_opens.load();
}

int FfmpegDemuxer::refuseIoOpen(AVFormatContext* /*s*/, AVIOContext** pb, const char* /*url*/,
                                int /*flags*/, AVDictionary** /*options*/) {
    // ADR-049 : aucune ouverture hors du fichier du job (listes HLS/DASH, concat, references
    // externes MOV, protocoles reseau). L'URL n'est pas journalisee (SEC-14).
    if (pb != nullptr) {
        *pb = nullptr;
    }
    g_refused_opens.fetch_add(1);
    logLine(LogLevel::Warn, "SecondaryOpenRefused",
            "the source asked to open another file or URL; refused (ADR-049)");
    return AVERROR(EPERM);
}

Result<std::unique_ptr<FfmpegDemuxer>> FfmpegDemuxer::open(const std::filesystem::path& path) {
    installFfmpegLogBridge();
    auto io = AvioReader::open(path);
    if (!io) {
        return enh::Unexpected<PipelineError>(std::move(io.error()));
    }
    auto self = std::make_unique<FfmpegDemuxer>(PrivateTag{});
    self->io_ = std::move(*io);

    FormatContextPtr alloc(avformat_alloc_context());
    if (!alloc) {
        return fail(PipelineErrorCode::Internal, "avformat_alloc_context failed");
    }
    alloc->pb = self->io_->context();
    alloc->flags |= AVFMT_FLAG_CUSTOM_IO;
    alloc->io_open = &FfmpegDemuxer::refuseIoOpen;

    AVDictionary* raw_opts = nullptr;
    av_dict_set(&raw_opts, "protocol_whitelist", "file", 0); // defense en profondeur
    DictionaryPtr opts(raw_opts);

    // Sondage AVANT l'ouverture : certains demuxeurs ouvrent d'autres fichiers par leurs propres
    // contextes imbriques, sans passer par notre io_open (concat, hls, dash, imf...). On les
    // refuse avant qu'ils ne lisent leur en-tete (ADR-049).
    const AVInputFormat* format = nullptr;
    if (const int r = av_probe_input_buffer2(self->io_->context(), &format, "", nullptr, 0, 0);
        r < 0 || format == nullptr) {
        if (self->io_->lastIoError()) {
            return enh::Unexpected<PipelineError>(*self->io_->lastIoError());
        }
        return enh::Unexpected<PipelineError>(
            avError(PipelineErrorCode::UnsupportedFormat, "av_probe_input_buffer2", r));
    }
    if (!isAllowedDemuxer(format->name)) {
        logLine(LogLevel::Warn, "DemuxerRefused",
                std::string("demuxer '") + format->name + "' is not in the allow list");
        return fail(PipelineErrorCode::UnsupportedFormat,
                    std::string("demuxer not allowed: ") + format->name);
    }

    // avformat_open_input libere le contexte en cas d'echec : on lui en cede la propriete.
    AVFormatContext* ctx = alloc.release();
    raw_opts = opts.release();
    const int open_r = avformat_open_input(&ctx, "", format, &raw_opts);
    opts.reset(raw_opts);
    if (open_r < 0) {
        if (self->io_->lastIoError()) {
            return enh::Unexpected<PipelineError>(*self->io_->lastIoError());
        }
        return enh::Unexpected<PipelineError>(
            avError(PipelineErrorCode::UnsupportedFormat, "avformat_open_input", open_r));
    }
    self->fmt_.reset(ctx);

    // Bornes des dimensions des l'en-tete du conteneur quand il les donne (avant l'analyse, qui
    // decode deja des images).
    for (unsigned i = 0; i < ctx->nb_streams; ++i) {
        const AVStream* st = ctx->streams[i];
        if (isUsableVideo(st) && st->codecpar->width > 0 && st->codecpar->height > 0) {
            if (auto r = checkStreamDimensions(*st->codecpar); !r) {
                return enh::Unexpected<PipelineError>(std::move(r.error()));
            }
        }
    }

    if (const int r = avformat_find_stream_info(ctx, nullptr); r < 0) {
        if (self->io_->lastIoError()) {
            return enh::Unexpected<PipelineError>(*self->io_->lastIoError());
        }
        return enh::Unexpected<PipelineError>(
            avError(PipelineErrorCode::UnsupportedFormat, "avformat_find_stream_info", r));
    }

    int index = av_find_best_stream(ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (index < 0 || !isUsableVideo(ctx->streams[index])) {
        index = -1;
        for (unsigned i = 0; i < ctx->nb_streams; ++i) {
            if (isUsableVideo(ctx->streams[i])) {
                index = static_cast<int>(i);
                break;
            }
        }
    }
    if (index < 0) {
        return fail(PipelineErrorCode::UnsupportedFormat, "no video stream");
    }
    for (unsigned i = 0; i < ctx->nb_streams; ++i) {
        if (static_cast<int>(i) != index) {
            ctx->streams[i]->discard = AVDISCARD_ALL; // pistes copiees : plus tard (ENH-013)
        }
    }

    const AVStream* st = ctx->streams[index];
    const AVCodecParameters& par = *st->codecpar;
    if (auto r = checkStreamDimensions(par); !r) {
        return enh::Unexpected<PipelineError>(std::move(r.error()));
    }

    VideoStreamInfo& info = self->info_;
    info.stream_index = index;
    info.codec = toSourceCodec(par.codec_id);
    info.codec_name = avcodec_get_name(par.codec_id);
    info.profile = par.profile;
    info.coded_size_px =
        Size{static_cast<std::uint32_t>(par.width), static_cast<std::uint32_t>(par.height)};
    const PixelFormatTraits traits = pixelFormatTraits(static_cast<AVPixelFormat>(par.format));
    if (traits.known) {
        info.bit_depth = traits.bit_depth;
        info.chroma = traits.chroma;
    } else {
        info.bit_depth = par.bits_per_raw_sample > 0 ? par.bits_per_raw_sample : 8;
        info.chroma = ChromaSubsampling::Yuv420;
    }
    info.time_base = toRational(st->time_base);
    info.avg_fps = toRational(st->avg_frame_rate.num > 0 ? st->avg_frame_rate : st->r_frame_rate);
    if (st->duration > 0 && st->time_base.den > 0) {
        info.duration = std::chrono::duration<double>(static_cast<double>(st->duration) *
                                                      av_q2d(st->time_base));
    } else if (ctx->duration > 0) {
        info.duration = std::chrono::duration<double>(static_cast<double>(ctx->duration) /
                                                      static_cast<double>(AV_TIME_BASE));
    }
    info.frame_count_estimate = st->nb_frames;
    info.color = toColorInfo(par.color_space, par.color_primaries, par.color_trc, par.color_range);
    return self;
}

const AVCodecParameters& FfmpegDemuxer::codecParameters() const noexcept {
    return *fmt_->streams[info_.stream_index]->codecpar;
}

AVRational FfmpegDemuxer::streamTimeBase() const noexcept {
    return fmt_->streams[info_.stream_index]->time_base;
}

PipelineError FfmpegDemuxer::readError(int averror) const {
    if (io_->lastIoError()) {
        return *io_->lastIoError();
    }
    PipelineError e = avError(PipelineErrorCode::SourceCorrupt, "av_read_frame", averror);
    if (last_pts_ != kNoPts) {
        e.at = std::chrono::duration<double>(static_cast<double>(last_pts_) *
                                             av_q2d(streamTimeBase()));
    }
    return e;
}

Result<std::optional<Packet>> FfmpegDemuxer::read() {
    for (;;) {
        PacketPtr pkt(av_packet_alloc());
        if (!pkt) {
            return fail(PipelineErrorCode::Internal, "av_packet_alloc failed");
        }
        const int r = av_read_frame(fmt_.get(), pkt.get());
        if (r == AVERROR_EOF) {
            if (io_->lastIoError()) {
                return enh::Unexpected<PipelineError>(*io_->lastIoError());
            }
            return std::optional<Packet>{};
        }
        if (r < 0) {
            return enh::Unexpected<PipelineError>(readError(r));
        }
        if (pkt->stream_index != info_.stream_index) {
            continue;
        }
        Packet out;
        out.pts = pkt->pts == AV_NOPTS_VALUE ? kNoPts : pkt->pts;
        out.dts = pkt->dts == AV_NOPTS_VALUE ? kNoPts : pkt->dts;
        out.duration = pkt->duration;
        out.time_base = info_.time_base;
        out.keyframe = (pkt->flags & AV_PKT_FLAG_KEY) != 0;
        out.corrupt = (pkt->flags & AV_PKT_FLAG_CORRUPT) != 0;
        if (out.pts != kNoPts) {
            last_pts_ = out.pts;
        }
        out.payload = std::make_unique<FfmpegPacketPayload>(std::move(pkt));
        return std::optional<Packet>(std::move(out));
    }
}

Result<void> FfmpegDemuxer::seek(std::chrono::duration<double> t) {
    const AVRational tb = streamTimeBase();
    if (tb.num <= 0 || tb.den <= 0 || !std::isfinite(t.count()) || t.count() < 0.0) {
        return fail(PipelineErrorCode::InvalidParams, "invalid seek target");
    }
    const double ticks = t.count() / av_q2d(tb);
    if (ticks > 9.0e18) { // hors de std::int64_t : conversion indefinie
        return fail(PipelineErrorCode::InvalidParams, "seek target out of range");
    }
    const auto ts = static_cast<std::int64_t>(ticks);
    if (const int r = av_seek_frame(fmt_.get(), info_.stream_index, ts, AVSEEK_FLAG_BACKWARD);
        r < 0) {
        return enh::Unexpected<PipelineError>(readError(r));
    }
    return {};
}

} // namespace enh::pipeline::ffmpeg
