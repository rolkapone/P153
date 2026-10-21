#include "enh/pipeline/gpu/nvdec_decoder.hpp"

#include <utility>

namespace enh::pipeline::gpu {

Result<std::unique_ptr<ffmpeg::FfmpegDecoder>>
createNvdecDecoder(const ffmpeg::FfmpegDemuxer& demuxer, std::shared_ptr<CudaFrameStore> store,
                   const ffmpeg::FfmpegDecoderOptions& options) {
    return ffmpeg::FfmpegDecoder::create(demuxer.codecParameters(), demuxer.streamTimeBase(),
                                         demuxer.videoStream(), std::move(store), options);
}

Result<std::unique_ptr<ffmpeg::FfmpegDecoder>>
createNvdecDecoder(const ffmpeg::FfmpegDemuxer& demuxer, const NvdecDecoderOptions& options) {
    auto store = CudaFrameStore::create(options.store);
    if (!store) {
        return enh::Unexpected<PipelineError>(std::move(store.error()));
    }
    return createNvdecDecoder(demuxer, std::move(*store), options.decoder);
}

} // namespace enh::pipeline::gpu
