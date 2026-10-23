#pragma once

// Decodeur (interface INTERNE au pipeline, docs/gpu/architecture-pipeline.md § 3.2).

#include "enh/pipeline/stage/packet.hpp"

#include <cstdint>
#include <stop_token>

namespace enh::pipeline {

/// Nvdec : images decodees par NVDEC (surfaces GPU) ; Software : decodage CPU (repli).
enum class DecoderPath : std::uint8_t { Nvdec, Software };

class IDecoder {
public:
    IDecoder() = default;
    IDecoder(const IDecoder&) = delete;
    IDecoder& operator=(const IDecoder&) = delete;
    IDecoder(IDecoder&&) = delete;
    IDecoder& operator=(IDecoder&&) = delete;
    virtual ~IDecoder() = default;

    /// Decode un paquet et pousse dans `out` les images disponibles. pkt == nullptr : vidange
    /// (fin de flux), toutes les images restantes sont poussees.
    virtual Result<void> decode(const Packet* pkt, IFrameSink& out, std::stop_token st) = 0;

    /// Chemin effectif (celui des dernieres images sorties ; avant la premiere : chemin prevu).
    [[nodiscard]] virtual DecoderPath path() const noexcept = 0;

    /// Paquets illisibles ignores (images masquees, avertissement CorruptFramesConcealed).
    [[nodiscard]] virtual std::uint64_t corruptPackets() const noexcept { return 0; }
};

} // namespace enh::pipeline
