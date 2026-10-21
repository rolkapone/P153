#include "enh/pipeline/core/log.hpp"

#include <mutex>
#include <string>
#include <utility>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace enh::pipeline {

namespace {

std::mutex& sinkMutex() {
    static std::mutex m;
    return m;
}

LogSink& currentSink() {
    static LogSink sink;
    return sink;
}

std::string_view levelName(LogLevel level) noexcept {
    switch (level) {
    case LogLevel::Debug:
        return "debug";
    case LogLevel::Info:
        return "info";
    case LogLevel::Warn:
        return "warn";
    case LogLevel::Error:
        return "error";
    }
    return "?";
}

} // namespace

void setLogSink(LogSink sink) {
    const std::scoped_lock lock(sinkMutex());
    currentSink() = std::move(sink);
}

void logLine(LogLevel level, std::string_view code, std::string_view message) noexcept {
    try {
        const std::scoped_lock lock(sinkMutex());
        if (currentSink()) {
            currentSink()(level, code, message);
            return;
        }
        std::string line = "[enhance:pipeline] ";
        line += levelName(level);
        line += ' ';
        line += code;
        line += ": ";
        line += message;
        line += '\n';
        OutputDebugStringA(line.c_str());
    } catch (...) { // NOLINT(bugprone-empty-catch) : le journal ne doit jamais faire echouer
    }
}

} // namespace enh::pipeline
