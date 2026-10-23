#include "enh/pipeline/core/log.hpp"

#include <memory>
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

std::shared_ptr<const LogSink>& currentSink() {
    static std::shared_ptr<const LogSink> sink;
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
    auto next = sink ? std::make_shared<const LogSink>(std::move(sink)) : nullptr;
    const std::scoped_lock lock(sinkMutex());
    currentSink() = std::move(next);
}

void logLine(LogLevel level, std::string_view code, std::string_view message) noexcept {
    try {
        std::shared_ptr<const LogSink> sink;
        {
            const std::scoped_lock lock(sinkMutex());
            sink = currentSink();
        }
        if (sink) {
            (*sink)(level, code, message); // hors verrou : un recepteur lent ne bloque personne
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
