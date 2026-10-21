#pragma once

// RAII des objets FFmpeg (docs/gpu/architecture-pipeline.md § 3.5) : un std::unique_ptr par type,
// avec la fonction de liberation FFmpeg comme suppresseur. Aucune liberation manuelle ailleurs.

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/buffer.h>
#include <libavutil/dict.h>
#include <libavutil/frame.h>
#include <libswscale/swscale.h>
}

#include <memory>

namespace enh::pipeline::ffmpeg {

struct FormatInputDeleter {
    void operator()(AVFormatContext* p) const noexcept { avformat_close_input(&p); }
};
/// Contexte ouvert par avformat_open_input (fermeture : avformat_close_input).
using FormatInputPtr = std::unique_ptr<AVFormatContext, FormatInputDeleter>;

struct FormatContextDeleter {
    void operator()(AVFormatContext* p) const noexcept { avformat_free_context(p); }
};
/// Contexte alloue mais pas encore ouvert (liberation : avformat_free_context).
using FormatContextPtr = std::unique_ptr<AVFormatContext, FormatContextDeleter>;

struct CodecContextDeleter {
    void operator()(AVCodecContext* p) const noexcept { avcodec_free_context(&p); }
};
using CodecContextPtr = std::unique_ptr<AVCodecContext, CodecContextDeleter>;

struct PacketDeleter {
    void operator()(AVPacket* p) const noexcept { av_packet_free(&p); }
};
using PacketPtr = std::unique_ptr<AVPacket, PacketDeleter>;

struct FrameDeleter {
    void operator()(AVFrame* p) const noexcept { av_frame_free(&p); }
};
using FramePtr = std::unique_ptr<AVFrame, FrameDeleter>;

struct BufferRefDeleter {
    void operator()(AVBufferRef* p) const noexcept { av_buffer_unref(&p); }
};
/// Reference comptee FFmpeg (ex. contexte materiel CUDA).
using BufferRefPtr = std::unique_ptr<AVBufferRef, BufferRefDeleter>;

struct DictionaryDeleter {
    void operator()(AVDictionary* p) const noexcept { av_dict_free(&p); }
};
using DictionaryPtr = std::unique_ptr<AVDictionary, DictionaryDeleter>;

struct SwsContextDeleter {
    void operator()(SwsContext* p) const noexcept { sws_freeContext(p); }
};
using SwsContextPtr = std::unique_ptr<SwsContext, SwsContextDeleter>;

} // namespace enh::pipeline::ffmpeg
