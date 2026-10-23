#pragma once

// Fabrique du decodeur reel : FfmpegDecoder + CudaFrameStore (NVDEC, images en memoire GPU).

#include "enh/pipeline/ffmpeg/ffmpeg_decoder.hpp"
#include "enh/pipeline/ffmpeg/ffmpeg_demuxer.hpp"
#include "enh/pipeline/gpu/cuda_frame_store.hpp"

#include <memory>

namespace enh::pipeline::gpu {

struct NvdecDecoderOptions {
    CudaFrameStoreOptions store;
    ffmpeg::FfmpegDecoderOptions decoder;
};

/// Decodeur pour la piste video de `demuxer`. Sans carte ou sans pilote : ComponentMissing.
[[nodiscard]] Result<std::unique_ptr<ffmpeg::FfmpegDecoder>>
createNvdecDecoder(const ffmpeg::FfmpegDemuxer& demuxer, const NvdecDecoderOptions& options = {});

/// Variante avec un magasin existant (tests : verification du contexte partage).
[[nodiscard]] Result<std::unique_ptr<ffmpeg::FfmpegDecoder>>
createNvdecDecoder(const ffmpeg::FfmpegDemuxer& demuxer, std::shared_ptr<CudaFrameStore> store,
                   const ffmpeg::FfmpegDecoderOptions& options = {});

} // namespace enh::pipeline::gpu
