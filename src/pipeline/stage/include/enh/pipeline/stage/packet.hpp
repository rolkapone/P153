#pragma once

// Paquet compresse entre demuxeur et decodeur (interface INTERNE au pipeline).

#include "enh/pipeline/stage/frame.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

namespace enh::pipeline {

/// Contenu d'un paquet. Le demuxeur FFmpeg y range un AVPacket (natif, sans copie) ; les faux
/// y rangent des octets en memoire.
class PacketPayload {
public:
    PacketPayload() = default;
    PacketPayload(const PacketPayload&) = delete;
    PacketPayload& operator=(const PacketPayload&) = delete;
    PacketPayload(PacketPayload&&) = delete;
    PacketPayload& operator=(PacketPayload&&) = delete;
    virtual ~PacketPayload() = default;

    [[nodiscard]] virtual std::span<const std::byte> bytes() const noexcept = 0;
    /// Objet natif (AVPacket* pour le demuxeur FFmpeg), nullptr sinon.
    [[nodiscard]] virtual void* native() noexcept { return nullptr; }
};

struct Packet {
    std::unique_ptr<PacketPayload> payload;
    std::int64_t pts = kNoPts; // dans time_base
    std::int64_t dts = kNoPts;
    std::int64_t duration = 0;
    Rational time_base;
    bool keyframe = false;
    bool corrupt = false; // signale par le demuxeur (AV_PKT_FLAG_CORRUPT)
};

} // namespace enh::pipeline
