#include "render_policy.hpp"

#include "gpu_frame_timer.hpp"

#include <QLoggingCategory>
#include <QQuickWindow>

#include <chrono>

namespace enh::app {

namespace {
Q_LOGGING_CATEGORY(lcRender, "enh.render", QtWarningMsg)
} // namespace

RenderPolicy::RenderPolicy(QObject* parent) : QObject(parent) {}

RenderPolicy::~RenderPolicy() {
    detachWindow();
}

int RenderPolicy::glassQuality() const noexcept {
    return static_cast<int>(options_.forcedGlassQuality.value_or(governed_));
}

void RenderPolicy::setCaptionColor(const QColor& color) {
    if (captionColor_ == color) {
        return;
    }
    captionColor_ = color;
    emit captionColorChanged();
}

void RenderPolicy::configure(const AppOptions& options, bool micaSupported) {
    const int oldQuality = glassQuality();
    const bool oldTestbed = options_.glassTestbed;
    options_ = options;
    if (glassQuality() != oldQuality) {
        emit glassQualityChanged();
    }
    if (options_.glassTestbed != oldTestbed) {
        emit glassTestbedChanged();
    }
    setMicaUnavailable(!micaSupported || options.noMica);
}

void RenderPolicy::setMicaUnavailable(bool unavailable) {
    if (micaUnavailable_ == unavailable) {
        return;
    }
    micaUnavailable_ = unavailable;
    emit micaUnavailableChanged();
}

void RenderPolicy::setProcessing(bool processing) {
    processing_ = processing;
    if (!processing_) {
        setGovernedQuality(governor_.update(GlassQualityGovernor::Milliseconds{0.0}, false,
                                            GlassQualityGovernor::Clock::now()));
    }
}

void RenderPolicy::setGovernedQuality(GlassQuality quality) {
    const int old = glassQuality();
    governed_ = quality;
    if (glassQuality() != old) {
        emit glassQualityChanged();
    }
}

void RenderPolicy::reportGpuFrameTime(double milliseconds) {
    const auto now = GlassQualityGovernor::Clock::now();
    setGovernedQuality(
        governor_.update(GlassQualityGovernor::Milliseconds{milliseconds}, processing_, now));

    // Moyenne par seconde, publiee pour la QA (QT_LOGGING_RULES="enh.render.info=true").
    if (windowFrames_ == 0) {
        windowStart_ = now;
    }
    windowSumMs_ += milliseconds;
    ++windowFrames_;
    if (now - windowStart_ >= std::chrono::seconds{1}) {
        uiGpuMs_ = windowSumMs_ / windowFrames_;
        qCInfo(lcRender, "GPU interface : %.3f ms en moyenne sur %d images (verre %d)", uiGpuMs_,
               windowFrames_, glassQuality());
        windowSumMs_ = 0.0;
        windowFrames_ = 0;
        emit uiGpuMsChanged();
    }
}

void RenderPolicy::attachWindow(QQuickWindow* window) {
    detachWindow();
    if (window == nullptr) {
        return;
    }
    window_ = window;
    // Le rappel tourne sur le thread de rendu : la mesure est postee au thread de l'interface.
    // Le contexte `this` annule l'appel si la politique a ete detruite entre-temps.
    timer_ = std::make_unique<GpuFrameTimer>([this](double ms) {
        QMetaObject::invokeMethod(
            this, [this, ms] { reportGpuFrameTime(ms); }, Qt::QueuedConnection);
    });
    GpuFrameTimer* timer = timer_.get();
    beforeConnection_ = connect(
        window, &QQuickWindow::beforeRendering, window,
        [timer, window] { timer->beginFrame(window); }, Qt::DirectConnection);
    afterConnection_ = connect(
        window, &QQuickWindow::afterRendering, window, [timer, window] { timer->endFrame(window); },
        Qt::DirectConnection);
    invalidatedConnection_ = connect(
        window, &QQuickWindow::sceneGraphInvalidated, window, [timer] { timer->release(); },
        Qt::DirectConnection);
}

void RenderPolicy::detachWindow() {
    disconnect(beforeConnection_);
    disconnect(afterConnection_);
    disconnect(invalidatedConnection_);
    window_.clear();
    timer_.reset();
}

} // namespace enh::app
