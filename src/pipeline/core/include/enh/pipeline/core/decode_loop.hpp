#pragma once

// Boucle demux -> decodage (version mono-thread, utilisee par les tests et le banc ; le pipeline
// complet la repartit sur T0/T1, ADR-045). Logique pure sur les interfaces : testee avec des faux.

#include "enh/pipeline/stage/decoder.hpp"
#include "enh/pipeline/stage/demuxer.hpp"

#include <chrono>
#include <cstdint>
#include <stop_token>

namespace enh::pipeline {

struct DecodeStats {
    std::uint64_t packets = 0;
    std::uint64_t frames = 0;
    std::uint64_t corrupt_packets = 0;
    DecoderPath path = DecoderPath::Software;
    std::chrono::duration<double> elapsed{0.0};
};

/// Lit tous les paquets, les decode, vidange le decodeur. Premiere erreur rendue telle quelle ;
/// arret de st -> Cancelled. `max_frames` > 0 : arret apres ce nombre d'images (sans vidange).
[[nodiscard]] Result<DecodeStats> runDecodeLoop(IDemuxer& demuxer, IDecoder& decoder,
                                                IFrameSink& sink, std::stop_token st,
                                                std::uint64_t max_frames = 0);

} // namespace enh::pipeline
