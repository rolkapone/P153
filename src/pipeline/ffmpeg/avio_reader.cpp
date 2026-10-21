#include "enh/pipeline/ffmpeg/avio_reader.hpp"

extern "C" {
#include <libavformat/avio.h>
#include <libavutil/error.h>
#include <libavutil/mem.h>
}

#include <cerrno>
#include <cstdio>
#include <span>

namespace enh::pipeline::ffmpeg {

Result<std::unique_ptr<AvioReader>> AvioReader::open(const std::filesystem::path& path) {
    auto file = Win32File::openRead(path);
    if (!file) {
        return enh::Unexpected<PipelineError>(std::move(file.error()));
    }
    auto reader = std::make_unique<AvioReader>(PrivateTag{}, std::move(*file));
    auto* buffer = static_cast<unsigned char*>(av_malloc(kBufferBytes));
    if (buffer == nullptr) {
        return fail(PipelineErrorCode::Internal, "av_malloc failed for AVIO buffer");
    }
    reader->ctx_ = avio_alloc_context(buffer, kBufferBytes, /*write_flag=*/0, reader.get(),
                                      &AvioReader::readPacket, nullptr, &AvioReader::seekCallback);
    if (reader->ctx_ == nullptr) {
        av_free(buffer);
        return fail(PipelineErrorCode::Internal, "avio_alloc_context failed");
    }
    return reader;
}

AvioReader::~AvioReader() {
    if (ctx_ != nullptr) {
        av_freep(static_cast<void*>(&ctx_->buffer)); // tampon courant (peut differer de l'initial)
        avio_context_free(&ctx_);
    }
}

int AvioReader::readPacket(void* opaque, std::uint8_t* buf, int buf_size) noexcept {
    auto* self = static_cast<AvioReader*>(opaque);
    if (buf_size <= 0) {
        return 0;
    }
    auto got = self->file_.read(std::span<std::byte>(reinterpret_cast<std::byte*>(buf),
                                                     static_cast<std::size_t>(buf_size)));
    if (!got) {
        self->last_error_ = got.error();
        return AVERROR(EIO);
    }
    if (*got == 0) {
        return AVERROR_EOF;
    }
    self->bytes_read_ += static_cast<std::int64_t>(*got);
    return static_cast<int>(*got);
}

std::int64_t AvioReader::seekCallback(void* opaque, std::int64_t offset, int whence) noexcept {
    auto* self = static_cast<AvioReader*>(opaque);
    const int mode = whence & ~AVSEEK_FORCE;
    if (mode == AVSEEK_SIZE) {
        auto sz = self->file_.size();
        if (!sz) {
            self->last_error_ = sz.error();
            return AVERROR(EIO);
        }
        return *sz;
    }
    SeekOrigin origin = SeekOrigin::Begin;
    if (mode == SEEK_CUR) {
        origin = SeekOrigin::Current;
    } else if (mode == SEEK_END) {
        origin = SeekOrigin::End;
    } else if (mode != SEEK_SET) {
        return AVERROR(EINVAL);
    }
    auto pos = self->file_.seek(offset, origin);
    if (!pos) {
        self->last_error_ = pos.error();
        return AVERROR(EIO);
    }
    return *pos;
}

} // namespace enh::pipeline::ffmpeg
