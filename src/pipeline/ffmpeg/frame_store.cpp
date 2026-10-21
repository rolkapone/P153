#include "enh/pipeline/ffmpeg/frame_store.hpp"

#include <utility>

namespace enh::pipeline::ffmpeg {

Result<FrameBufferPtr> IFrameStore::importDevice(const AVFrame& /*frame*/,
                                                 const FrameDesc& /*desc*/,
                                                 std::stop_token /*st*/) {
    return fail(PipelineErrorCode::Internal, "this frame store does not accept GPU frames");
}

Result<FrameBufferPtr> HostFrameStore::acquireStaging(const FrameDesc& desc, std::stop_token st) {
    if (!pool_ || pool_->desc() != desc) {
        auto pool = HostFramePool::create(desc, capacity_);
        if (!pool) {
            return enh::Unexpected<PipelineError>(std::move(pool.error()));
        }
        pool_ = std::move(*pool);
    }
    return pool_->acquire(std::move(st));
}

Result<FrameBufferPtr> HostFrameStore::commit(FrameBufferPtr staging, std::stop_token /*st*/) {
    return staging;
}

} // namespace enh::pipeline::ffmpeg
