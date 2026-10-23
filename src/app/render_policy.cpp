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
    // Thread de rendu : les trois connexions directes ci-dessous ; chacune capture le
    // shared_ptr PAR VALEUR, donc une image en cours garde le minuteur vivant meme si
    // detachWindow() est appele au meme moment depuis le thread de l'interface. Aucune de ces
    // fonctions ne touche `this` : la politique peut etre detruite sans risque.
    // Thread de l'interface : les mesures sont relevees apres chaque image affichee
    // (connexion en file d'attente, annulee automatiquement si `this` est detruit).
    timer_ = std::make_shared<GpuFrameTimer>();
    const std::shared_ptr<GpuFrameTimer> timer = timer_;
    beforeConnection_ = connect(
        window, &QQuickWindow::beforeRendering, window,
        [timer, window] { timer->beginFrame(window); }, Qt::DirectConnection);
    afterConnection_ = connect(
        window, &QQuickWindow::afterRendering, window, [timer, window] { timer->endFrame(window); },
        Qt::DirectConnection);
    invalidatedConnection_ = connect(
        window, &QQuickWindow::sceneGraphInvalidated, window, [timer] { timer->release(); },
        Qt::DirectConnection);
    swappedConnection_ = connect(window, &QQuickWindow::frameSwapped, this,
                                 &RenderPolicy::collectGpuResults, Qt::QueuedConnection);
}

void RenderPolicy::collectGpuResults() {
    if (!timer_) {
        return;
    }
    for (const double ms : timer_->takeResults()) {
        reportGpuFrameTime(ms);
    }
}

void RenderPolicy::detachWindow() {
    disconnect(beforeConnection_);
    disconnect(afterConnection_);
    disconnect(invalidatedConnection_);
    disconnect(swappedConnection_);
    window_.clear();
    // Derniere reference cote interface ; une image deja lancee garde la sienne jusqu'a la fin.
    timer_.reset();
}

} // namespace enh::app
