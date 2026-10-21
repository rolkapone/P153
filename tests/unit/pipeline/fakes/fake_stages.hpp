#pragma once

// Faux etages pour la CI (docs/gpu/ENH-008.md § 3) : FakeDemuxer, FakeDecoder, CollectingSink.

#include "enh/pipeline/core/host_frame_pool.hpp"
#include "enh/pipeline/stage/decoder.hpp"
#include "enh/pipeline/stage/demuxer.hpp"

#include <cstdint>
#include <deque>
#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace enh::pipeline::fakes {

class BytesPayload final : public PacketPayload {
public:
    explicit BytesPayload(std::vector<std::byte> b) : bytes_(std::move(b)) {}
    [[nodiscard]] std::span<const std::byte> bytes() const noexcept override { return bytes_; }

private:
    std::vector<std::byte> bytes_;
};

/// `count` paquets numerotes (octet 0 = numero), erreur injectee au paquet `fail_at` (si >= 0).
class FakeDemuxer final : public IDemuxer {
public:
    explicit FakeDemuxer(int count, int fail_at = -1,
                         PipelineErrorCode code = PipelineErrorCode::SourceCorrupt)
        : count_(count), fail_at_(fail_at), code_(code) {
        info_.stream_index = 0;
        info_.codec = SourceCodec::H264;
        info_.codec_name = "fake";
        info_.coded_size_px = Size{64, 36};
        info_.time_base = Rational{1, 30};
        info_.avg_fps = Rational{30, 1};
    }
    [[nodiscard]] const VideoStreamInfo& videoStream() const noexcept override { return info_; }
    Result<std::optional<Packet>> read() override {
        if (next_ == fail_at_) {
            return fail(code_, "injected demux error");
        }
        if (next_ >= count_) {
            return std::optional<Packet>{};
        }
        Packet p;
        p.payload = std::make_unique<BytesPayload>(
            std::vector<std::byte>{static_cast<std::byte>(next_ & 0xFF)});
        p.pts = next_;
        p.time_base = info_.time_base;
        p.keyframe = (next_ % 10) == 0;
        ++next_;
        return std::optional<Packet>(std::move(p));
    }
    Result<void> seek(std::chrono::duration<double> t) override {
        next_ = static_cast<int>(t.count() * 30.0);
        return {};
    }

private:
    VideoStreamInfo info_;
    int count_;
    int fail_at_;
    PipelineErrorCode code_;
    int next_ = 0;
};

/// Une image CPU par paquet, avec `delay` images retenues (comme un decodeur avec B-frames) :
/// elles ne sortent qu'a la vidange.
class FakeDecoder final : public IDecoder {
public:
    explicit FakeDecoder(std::size_t delay = 2) : delay_(delay) {
        auto pool = HostFramePool::create(FrameDesc{Size{64, 36}, PixelFormat::Nv12, {}}, 4);
        pool_ = std::move(*pool);
    }
    Result<void> decode(const Packet* pkt, IFrameSink& out, std::stop_token st) override {
        if (pkt != nullptr) {
            pending_.push_back(pkt->pts);
            if (pending_.size() <= delay_) {
                return {};
            }
        }
        while (!pending_.empty() && (pkt == nullptr || pending_.size() > delay_)) {
            auto buffer = pool_->acquire(st);
            if (!buffer) {
                return enh::Unexpected<PipelineError>(buffer.error());
            }
            VideoFrame f;
            f.desc = pool_->desc();
            f.pts = pending_.front();
            f.index = produced_++;
            f.buffer = std::move(*buffer);
            pending_.pop_front();
            if (auto r = out.push(std::move(f), st); !r) {
                return r;
            }
        }
        return {};
    }
    [[nodiscard]] DecoderPath path() const noexcept override { return DecoderPath::Software; }
    [[nodiscard]] const HostFramePool& pool() const noexcept { return *pool_; }

private:
    std::size_t delay_;
    std::shared_ptr<HostFramePool> pool_;
    std::deque<std::int64_t> pending_;
    std::int64_t produced_ = 0;
};

/// Garde les horodatages ; libere les images tout de suite (sinon le pool se viderait).
/// `stop_after` > 0 : demande l'arret a la N-ieme image.
class CollectingSink final : public IFrameSink {
public:
    explicit CollectingSink(std::stop_source* stop = nullptr, std::size_t stop_after = 0)
        : stop_(stop), stop_after_(stop_after) {}
    Result<void> push(VideoFrame&& frame, std::stop_token /*st*/) override {
        pts.push_back(frame.pts);
        memory.push_back(frame.buffer ? frame.buffer->memory() : MemoryKind::Host);
        formats.push_back(frame.desc.format);
        if (stop_ != nullptr && stop_after_ != 0 && pts.size() == stop_after_) {
            stop_->request_stop();
        }
        return {};
    }
    std::vector<std::int64_t> pts;
    std::vector<MemoryKind> memory;
    std::vector<PixelFormat> formats;

private:
    std::stop_source* stop_;
    std::size_t stop_after_;
};

} // namespace enh::pipeline::fakes
