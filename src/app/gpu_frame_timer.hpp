#pragma once

#include <functional>
#include <memory>

class QQuickWindow;

namespace enh::app {

/// Temps GPU de rendu d'une image Qt Quick, par requetes D3D11 « timestamp » (ENH-004 §5.4).
/// Toutes les methodes sont appelees sur le thread de rendu de Qt Quick. Les resultats sont lus
/// 2 a 3 images plus tard, sans jamais attendre le GPU (aucune attente bloquante).
/// Autre API graphique (rendu logiciel des tests, OpenGL) : la mesure est simplement inactive.
class GpuFrameTimer {
public:
    using ResultCallback = std::function<void(double milliseconds)>;

    explicit GpuFrameTimer(ResultCallback onResult);
    ~GpuFrameTimer();

    GpuFrameTimer(const GpuFrameTimer&) = delete;
    GpuFrameTimer& operator=(const GpuFrameTimer&) = delete;
    GpuFrameTimer(GpuFrameTimer&&) = delete;
    GpuFrameTimer& operator=(GpuFrameTimer&&) = delete;

    void beginFrame(QQuickWindow* window); ///< QQuickWindow::beforeRendering
    void endFrame(QQuickWindow* window);   ///< QQuickWindow::afterRendering
    void release();                        ///< QQuickWindow::sceneGraphInvalidated

private:
    struct D3D; // objets COM (pimpl : d3d11.h reste dans le .cpp)
    std::unique_ptr<D3D> d3d_;
    ResultCallback onResult_;
    bool unsupported_ = false;
};

} // namespace enh::app
