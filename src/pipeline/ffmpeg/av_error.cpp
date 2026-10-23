#include "enh/pipeline/ffmpeg/av_error.hpp"

#include "enh/pipeline/core/log.hpp"

extern "C" {
#include <libavutil/error.h>
#include <libavutil/log.h>
}

#include <array>
#include <cstdarg>
#include <mutex>

namespace enh::pipeline::ffmpeg {

std::string avErrorText(int averror) {
    std::array<char, AV_ERROR_MAX_STRING_SIZE> buf{};
    if (av_strerror(averror, buf.data(), buf.size()) < 0) {
        return "unknown FFmpeg error";
    }
    return std::string(buf.data());
}

PipelineError avError(PipelineErrorCode code, std::string_view what, int averror,
                      std::source_location where) {
    std::string detail(what);
    detail += ": ";
    detail += avErrorText(averror);
    detail += " (";
    detail += std::to_string(averror);
    detail += ')';
    return makeError(code, std::move(detail), averror, where);
}

namespace {

void ffmpegLogCallback(void* avcl, int level, const char* fmt, va_list vl) {
    if (level > av_log_get_level()) {
        return;
    }
    std::array<char, 1024> line{};
    int print_prefix = 1;
    if (av_log_format_line2(avcl, level, fmt, vl, line.data(), static_cast<int>(line.size()),
                            &print_prefix) < 0) {
        return;
    }
    std::string_view text(line.data());
    while (!text.empty() && (text.back() == '\n' || text.back() == '\r')) {
        text.remove_suffix(1);
    }
    if (text.empty()) {
        return;
    }
    LogLevel ours = LogLevel::Debug;
    if (level <= AV_LOG_ERROR) {
        ours = LogLevel::Error;
    } else if (level <= AV_LOG_WARNING) {
        ours = LogLevel::Warn;
    } else if (level <= AV_LOG_INFO) {
        ours = LogLevel::Info;
    }
    logLine(ours, "ffmpeg", text);
}

} // namespace

void installFfmpegLogBridge() noexcept {
    static std::once_flag once;
    try {
        std::call_once(once, [] {
            av_log_set_level(AV_LOG_WARNING);
            av_log_set_callback(&ffmpegLogCallback);
        });
    } catch (...) { // NOLINT(bugprone-empty-catch) : sans pont, FFmpeg ecrit sur stderr
    }
}

} // namespace enh::pipeline::ffmpeg
