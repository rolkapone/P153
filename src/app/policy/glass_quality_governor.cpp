#include "enh/app/glass_quality_governor.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace enh::app {
namespace {

GlassQuality oneStepUp(GlassQuality q) noexcept {
    return q == GlassQuality::Opaque ? GlassQuality::Lite : GlassQuality::Full;
}

} // namespace

GlassQuality GlassQualityGovernor::update(Milliseconds gpuFrameTime, bool processing,
                                          Clock::time_point now) noexcept {
    if (!processing) {
        reset();
        quality_ = GlassQuality::Full;
        return quality_;
    }
    const double ms = gpuFrameTime.count();
    if (!std::isfinite(ms) || ms < 0.0) {
        return quality_;
    }
    samples_[next_] = std::min(ms, kMaxSample.count());
    next_ = (next_ + 1) % kWindowFrames;
    filled_ = std::min(filled_ + 1, kWindowFrames);

    const auto avg = average();
    if (!avg) {
        return quality_;
    }
    if (*avg > kOpaqueAbove) {
        quality_ = GlassQuality::Opaque;
        calmSince_.reset();
    } else if (*avg > kLiteAbove) {
        if (quality_ == GlassQuality::Full) {
            quality_ = GlassQuality::Lite;
        }
        calmSince_.reset();
    } else if (*avg < kRecoverBelow && quality_ != GlassQuality::Full) {
        if (!calmSince_) {
            calmSince_ = now;
        } else if (now - *calmSince_ >= kRecoverAfter) {
            quality_ = oneStepUp(quality_);
            calmSince_ = now; // le cran suivant demande a nouveau 10 s
        }
    } else {
        calmSince_.reset(); // entre le budget et le seuil : on garde la qualite courante
    }
    return quality_;
}

std::optional<GlassQualityGovernor::Milliseconds> GlassQualityGovernor::average() const noexcept {
    if (filled_ < kWindowFrames) {
        return std::nullopt;
    }
    // Somme recalculee (30 valeurs) plutot qu'une somme courante : aucune derive d'arrondi.
    const double sum = std::accumulate(samples_.begin(), samples_.end(), 0.0);
    return Milliseconds{sum / static_cast<double>(kWindowFrames)};
}

void GlassQualityGovernor::reset() noexcept {
    samples_.fill(0.0);
    next_ = 0;
    filled_ = 0;
    calmSince_.reset();
}

} // namespace enh::app
