#pragma once

// Faux IUpscaler pour la CI (ENH-040) : memes regles de validation que RtxVideoUpscaler, compte
// les appels, erreur injectable a l'appel n (base 0). Ne touche a aucun GPU.

#include "enh/pipeline/stage/upscaler.hpp"

#include <optional>

namespace enh::pipeline::fakes {

class FakeUpscaler final : public IUpscaler {
public:
    explicit FakeUpscaler(int fail_at = -1, PipelineErrorCode code = PipelineErrorCode::OutOfVram)
        : fail_at_(fail_at), code_(code) {}

    [[nodiscard]] Result<void> upscale(const UpscaleRequest& request) noexcept override {
        if (request.input_texture == 0 || request.output_surface == 0 ||
            request.input_size_px.width == 0 || request.input_size_px.height == 0 ||
            request.output_size_px.width == 0 || request.output_size_px.height == 0 ||
            request.quality > UpscaleQuality::Ultra) {
            return fail(PipelineErrorCode::InvalidParams, "FakeUpscaler: invalid request");
        }
        const int index = calls_++;
        if (index == fail_at_) {
            return fail(code_, "FakeUpscaler: injected error");
        }
        last_ = request;
        return {};
    }

    [[nodiscard]] int calls() const noexcept { return calls_; }
    [[nodiscard]] const std::optional<UpscaleRequest>& last() const noexcept { return last_; }

private:
    int fail_at_;
    PipelineErrorCode code_;
    int calls_ = 0;
    std::optional<UpscaleRequest> last_;
};

} // namespace enh::pipeline::fakes
