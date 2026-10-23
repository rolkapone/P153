#include "enh/pipeline/core/decoder_selection.hpp"

namespace enh::pipeline {

namespace {

std::string_view softwareName(SourceCodec codec) noexcept {
    switch (codec) {
    case SourceCodec::H264:
        return "h264";
    case SourceCodec::Hevc:
        return "hevc";
    case SourceCodec::Av1:
        return "libdav1d"; // le decodeur natif "av1" ne decode pas sans hwaccel
    case SourceCodec::Vp9:
        return "vp9"; // natif (et non libvpx-vp9)
    case SourceCodec::Vp8:
        return "vp8";
    case SourceCodec::Mpeg2:
        return "mpeg2video";
    case SourceCodec::Mpeg1:
        return "mpeg1video";
    case SourceCodec::Mpeg4:
        return "mpeg4";
    case SourceCodec::Vc1:
        return "vc1";
    case SourceCodec::ProRes:
        return "prores";
    case SourceCodec::Ffv1:
        return "ffv1";
    case SourceCodec::Other:
        return {};
    }
    return {};
}

std::string_view hardwareName(SourceCodec codec) noexcept {
    return codec == SourceCodec::Av1 ? std::string_view{"av1"} : softwareName(codec);
}

} // namespace

int nvdecMaxBitDepth(SourceCodec codec) noexcept {
    switch (codec) {
    case SourceCodec::Hevc:
    case SourceCodec::Vp9:
        return 12;          // sortie P016 ramenee en P010 (bits de poids fort)
    case SourceCodec::H264: // High10 : Blackwell oui ; si FFmpeg refuse, get_format bascule
    case SourceCodec::Av1:
        return 10;
    case SourceCodec::Mpeg2:
    case SourceCodec::Mpeg1:
    case SourceCodec::Mpeg4:
    case SourceCodec::Vc1:
    case SourceCodec::Vp8:
        return 8;
    case SourceCodec::ProRes:
    case SourceCodec::Ffv1:
    case SourceCodec::Other:
        return 0;
    }
    return 0;
}

DecoderChoice selectDecoder(const VideoStreamInfo& stream, bool hardware_available) noexcept {
    DecoderChoice choice;
    const int max_bits = nvdecMaxBitDepth(stream.codec);
    if (!hardware_available) {
        choice.reason = "no-nvidia-gpu";
    } else if (max_bits == 0) {
        choice.reason = "nvdec-unsupported-codec";
    } else if (stream.chroma != ChromaSubsampling::Yuv420) {
        choice.reason = "nvdec-unsupported-chroma";
    } else if (stream.bit_depth > max_bits) {
        choice.reason = "nvdec-unsupported-bit-depth";
    } else {
        choice.try_hardware = true;
    }
    choice.decoder_name =
        choice.try_hardware ? hardwareName(stream.codec) : softwareName(stream.codec);
    return choice;
}

} // namespace enh::pipeline
