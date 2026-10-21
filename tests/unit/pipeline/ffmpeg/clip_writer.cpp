#include "clip_writer.hpp"

#include "enh/pipeline/core/win32_file.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
}

#include <array>
#include <cstdio>
#include <memory>
#include <system_error>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace enh::pipeline::testing {

namespace {

struct OutputCloser {
    void operator()(AVFormatContext* oc) const noexcept {
        if (oc->pb != nullptr && (oc->oformat->flags & AVFMT_NOFILE) == 0) {
            avio_closep(&oc->pb);
        }
        avformat_free_context(oc);
    }
};
struct CodecCloser {
    void operator()(AVCodecContext* c) const noexcept { avcodec_free_context(&c); }
};
struct FrameCloser {
    void operator()(AVFrame* f) const noexcept { av_frame_free(&f); }
};
struct PacketCloser {
    void operator()(AVPacket* p) const noexcept { av_packet_free(&p); }
};

std::string drain(AVCodecContext* enc, AVFormatContext* oc, AVStream* st, AVPacket* pkt) {
    for (;;) {
        const int r = avcodec_receive_packet(enc, pkt);
        if (r == AVERROR(EAGAIN) || r == AVERROR_EOF) {
            return {};
        }
        if (r < 0) {
            return "avcodec_receive_packet";
        }
        av_packet_rescale_ts(pkt, enc->time_base, st->time_base);
        pkt->stream_index = st->index;
        if (av_interleaved_write_frame(oc, pkt) < 0) {
            return "av_interleaved_write_frame";
        }
    }
}

} // namespace

std::string writeClip(const std::filesystem::path& path, const ClipSpec& spec) {
    const std::string utf8 = path.string();
    AVFormatContext* raw_oc = nullptr;
    if (avformat_alloc_output_context2(&raw_oc, nullptr, spec.muxer.c_str(), utf8.c_str()) < 0) {
        return "avformat_alloc_output_context2";
    }
    std::unique_ptr<AVFormatContext, OutputCloser> oc(raw_oc);
    const AVCodec* codec = avcodec_find_encoder_by_name(spec.encoder.c_str());
    if (codec == nullptr) {
        return "encoder not found: " + spec.encoder;
    }
    AVStream* st = avformat_new_stream(oc.get(), nullptr);
    std::unique_ptr<AVCodecContext, CodecCloser> enc(avcodec_alloc_context3(codec));
    enc->width = spec.width;
    enc->height = spec.height;
    enc->pix_fmt = spec.pix_fmt;
    enc->time_base = AVRational{1, spec.fps};
    enc->framerate = AVRational{spec.fps, 1};
    enc->gop_size = 12;
    if (spec.encoder == "mpeg2video" || spec.encoder == "mpeg4") {
        enc->max_b_frames = 2; // reordonnancement : vidange du decodeur exercee
    }
    if (spec.profile >= 0) {
        enc->profile = spec.profile;
    }
    if ((oc->oformat->flags & AVFMT_GLOBALHEADER) != 0) {
        enc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    if (avcodec_open2(enc.get(), codec, nullptr) < 0) {
        return "avcodec_open2 " + spec.encoder;
    }
    avcodec_parameters_from_context(st->codecpar, enc.get());
    st->time_base = enc->time_base;
    if (avio_open(&oc->pb, utf8.c_str(), AVIO_FLAG_WRITE) < 0) {
        return "avio_open";
    }
    if (avformat_write_header(oc.get(), nullptr) < 0) {
        return "avformat_write_header";
    }
    std::unique_ptr<AVFrame, FrameCloser> frame(av_frame_alloc());
    std::unique_ptr<AVPacket, PacketCloser> pkt(av_packet_alloc());
    frame->format = spec.pix_fmt;
    frame->width = spec.width;
    frame->height = spec.height;
    if (av_frame_get_buffer(frame.get(), 0) < 0) {
        return "av_frame_get_buffer";
    }
    for (int i = 0; i < spec.frames; ++i) {
        av_frame_make_writable(frame.get());
        // Motif mobile : un octet par position (suffit pour tous les formats planaires testes).
        for (int p = 0; p < 4 && frame->data[p] != nullptr; ++p) {
            const int rows = p == 0 ? spec.height : (spec.height + 1) / 2;
            for (int y = 0; y < rows; ++y) {
                for (int x = 0; x < frame->linesize[p]; ++x) {
                    frame->data[p][y * frame->linesize[p] + x] =
                        static_cast<std::uint8_t>(p == 0 ? (x + y + 3 * i) : 128 + ((x + i) & 7));
                }
            }
        }
        frame->pts = i;
        if (avcodec_send_frame(enc.get(), frame.get()) < 0) {
            return "avcodec_send_frame";
        }
        if (auto e = drain(enc.get(), oc.get(), st, pkt.get()); !e.empty()) {
            return e;
        }
    }
    avcodec_send_frame(enc.get(), nullptr);
    if (auto e = drain(enc.get(), oc.get(), st, pkt.get()); !e.empty()) {
        return e;
    }
    if (av_write_trailer(oc.get()) < 0) {
        return "av_write_trailer";
    }
    return {};
}

ScopedTempDir::ScopedTempDir() {
    path_ = std::filesystem::temp_directory_path() /
            ("enh-ffmpeg-" + std::to_string(GetCurrentProcessId()) + "-" +
             std::to_string(GetTickCount64()));
    std::filesystem::create_directories(path_);
}

ScopedTempDir::~ScopedTempDir() {
    if (GetEnvironmentVariableW(L"ENH_KEEP_TEST_CLIPS", nullptr, 0) != 0) {
        return; // diagnostic : garder les clips generes
    }
    std::error_code ec;
    std::filesystem::remove_all(std::filesystem::path(toExtendedPath(path_)), ec);
}

} // namespace enh::pipeline::testing

namespace enh::pipeline::testing {

std::optional<long long> ffprobeFrameCount(const std::wstring& ffprobe_exe,
                                           const std::filesystem::path& clip) {
    // cmd.exe /c : la commande entiere est entouree de guillemets supplementaires.
    const std::wstring cmd = L"\"\"" + ffprobe_exe +
                             L"\" -v error -count_frames -select_streams v:0 -show_entries "
                             L"stream=nb_read_frames -of csv=p=0 \"" +
                             clip.native() + L"\"\"";
    FILE* pipe = _wpopen(cmd.c_str(), L"r");
    if (pipe == nullptr) {
        return std::nullopt;
    }
    std::string out;
    std::array<char, 256> buf{};
    while (fgets(buf.data(), static_cast<int>(buf.size()), pipe) != nullptr) {
        out += buf.data();
    }
    if (_pclose(pipe) != 0) {
        return std::nullopt;
    }
    try {
        return std::stoll(out);
    } catch (...) {
        return std::nullopt;
    }
}

} // namespace enh::pipeline::testing
