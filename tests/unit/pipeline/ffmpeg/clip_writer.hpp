#pragma once

// Genere de petits clips de test avec la build FFmpeg liee (aucun clip commite, aucun reseau).

#include <filesystem>
#include <optional>
#include <string>

extern "C" {
#include <libavutil/pixfmt.h>
}

namespace enh::pipeline::testing {

struct ClipSpec {
    std::string muxer;   // "mpegts", "matroska", "avi", "mp4", "mov"
    std::string encoder; // "mpeg2video", "ffv1", "mpeg4", "prores_ks"
    AVPixelFormat pix_fmt = AV_PIX_FMT_YUV420P;
    int width = 160;
    int height = 90;
    int frames = 30;
    int fps = 25;
    int profile = -1; // ex. prores_ks : 3 = HQ
};

/// Ecrit le clip ; renvoie un message d'erreur (vide si OK). Chemin ASCII (avio).
std::string writeClip(const std::filesystem::path& path, const ClipSpec& spec);

/// Nombre d images selon `ffprobe -count_frames` (piste video 0) ; nullopt si echec.
std::optional<long long> ffprobeFrameCount(const std::wstring& ffprobe_exe,
                                           const std::filesystem::path& clip);

/// Dossier temporaire supprime a la destruction.
class ScopedTempDir {
public:
    ScopedTempDir();
    ScopedTempDir(const ScopedTempDir&) = delete;
    ScopedTempDir& operator=(const ScopedTempDir&) = delete;
    ~ScopedTempDir();
    [[nodiscard]] const std::filesystem::path& path() const { return path_; }

private:
    std::filesystem::path path_;
};

} // namespace enh::pipeline::testing
