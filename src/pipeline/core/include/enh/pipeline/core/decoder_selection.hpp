#pragma once

// Choix du decodeur (fonction pure, testee en CI) : decodeur FFmpeg par son NOM et tentative
// NVDEC ou non. Le decodeur AV1 natif "av1" est choisi quand NVDEC est tente : libdav1d n'a pas de
// hwaccel (docs/gpu/ENH-008.md § 1). Matrice NVDEC : NVDEC_Application_Note.pdf (Video Codec SDK
// 13.1), tableau 1 « NVDEC Hardware Capabilities », colonne Blackwell.

#include "enh/pipeline/stage/demuxer.hpp"

#include <string_view>

namespace enh::pipeline {

struct DecoderChoice {
    std::string_view decoder_name; // nom FFmpeg (avcodec_find_decoder_by_name) ; vide = defaut
    bool try_hardware = false;     // proposer AV_PIX_FMT_CUDA (NVDEC) a get_format
    std::string_view reason;       // si try_hardware == false : raison stable (journal)
};

/// Profondeur maximale tentee sur NVDEC pour un codec (0 = codec non gere par NVDEC).
[[nodiscard]] int nvdecMaxBitDepth(SourceCodec codec) noexcept;

/// `hardware_available` : un GPU NVIDIA et son contexte CUDA sont prets.
/// Regles : 4:2:0 seulement (NV12 / P010 en sortie ; 4:2:2 et 4:4:4 -> logiciel puis conversion
/// en 4:2:0, perte de chrominance documentee), profondeur <= nvdecMaxBitDepth(codec).
[[nodiscard]] DecoderChoice selectDecoder(const VideoStreamInfo& stream,
                                          bool hardware_available) noexcept;

} // namespace enh::pipeline
