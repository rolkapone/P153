#pragma once

// AVIOContext maison sur un Win32File (ADR-049) : FFmpeg ne recoit jamais de chemin texte.
// Classe RAII : le destructeur libere pb->buffer (FFmpeg a pu le reallouer) puis le contexte, puis
// ferme le fichier.

#include "enh/pipeline/core/win32_file.hpp"

#include <filesystem>
#include <memory>
#include <optional>

struct AVIOContext;

namespace enh::pipeline::ffmpeg {

class AvioReader {
public:
    static constexpr int kBufferBytes = 64 * 1024;

    [[nodiscard]] static Result<std::unique_ptr<AvioReader>>
    open(const std::filesystem::path& path);

    AvioReader(const AvioReader&) = delete;
    AvioReader& operator=(const AvioReader&) = delete;
    AvioReader(AvioReader&&) = delete;
    AvioReader& operator=(AvioReader&&) = delete;
    ~AvioReader();

    [[nodiscard]] AVIOContext* context() const noexcept { return ctx_; }
    /// Derniere erreur d'E/S Win32 vue par un rappel (permet de distinguer SourceMissing d'un
    /// fichier corrompu quand FFmpeg ne rend qu'un AVERROR generique).
    [[nodiscard]] const std::optional<PipelineError>& lastIoError() const noexcept {
        return last_error_;
    }
    /// Nombre d'octets lus (tests, statistiques).
    [[nodiscard]] std::int64_t bytesRead() const noexcept { return bytes_read_; }

private:
    struct PrivateTag {};

public:
    AvioReader(PrivateTag /*tag*/, Win32File file) noexcept : file_(std::move(file)) {}

private:
    static int readPacket(void* opaque, std::uint8_t* buf, int buf_size) noexcept;
    static std::int64_t seekCallback(void* opaque, std::int64_t offset, int whence) noexcept;
    static int readPacketImpl(void* opaque, std::uint8_t* buf, int buf_size);
    static std::int64_t seekImpl(void* opaque, std::int64_t offset, int whence);

    Win32File file_;
    AVIOContext* ctx_ = nullptr; // possede (libere dans le destructeur)
    std::optional<PipelineError> last_error_;
    std::int64_t bytes_read_ = 0;
};

} // namespace enh::pipeline::ffmpeg
