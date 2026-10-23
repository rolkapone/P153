#include "enh/pipeline/core/decode_loop.hpp"

#include <utility>

namespace enh::pipeline {

namespace {

/// Compte les images qui passent, puis les transmet.
class CountingSink final : public IFrameSink {
public:
    explicit CountingSink(IFrameSink& next) noexcept : next_(next) {}
    Result<void> push(VideoFrame&& frame, std::stop_token st) override {
        auto r = next_.push(std::move(frame), std::move(st));
        if (r) {
            ++count_;
        }
        return r;
    }
    [[nodiscard]] std::uint64_t count() const noexcept { return count_; }

private:
    IFrameSink& next_;
    std::uint64_t count_ = 0;
};

Result<DecodeStats> cancelled() {
    return fail(PipelineErrorCode::Cancelled, "decode loop cancelled");
}

} // namespace

Result<DecodeStats> runDecodeLoop(IDemuxer& demuxer, IDecoder& decoder, IFrameSink& sink,
                                  std::stop_token st, std::uint64_t max_frames) {
    const auto start = std::chrono::steady_clock::now();
    CountingSink counting(sink);
    DecodeStats stats;
    const auto limitReached = [&] { return max_frames != 0 && counting.count() >= max_frames; };

    while (!limitReached()) {
        if (st.stop_requested()) {
            return cancelled();
        }
        auto pkt = demuxer.read();
        if (!pkt) {
            return enh::Unexpected<PipelineError>(std::move(pkt.error()));
        }
        if (!pkt->has_value()) {
            break; // fin du fichier
        }
        ++stats.packets;
        if (auto r = decoder.decode(&**pkt, counting, st); !r) {
            return enh::Unexpected<PipelineError>(std::move(r.error()));
        }
    }
    if (!limitReached()) {
        if (st.stop_requested()) {
            return cancelled();
        }
        if (auto r = decoder.decode(nullptr, counting, st); !r) {
            return enh::Unexpected<PipelineError>(std::move(r.error()));
        }
    }
    stats.frames = counting.count();
    stats.corrupt_packets = decoder.corruptPackets();
    stats.path = decoder.path();
    stats.elapsed = std::chrono::steady_clock::now() - start;
    return stats;
}

} // namespace enh::pipeline
