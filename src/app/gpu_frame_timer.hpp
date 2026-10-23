#pragma once

#include <cstddef>
#include <memory>
#include <mutex>
#include <vector>

class QQuickWindow;

namespace enh::app {

/// Temps GPU de rendu d'une image Qt Quick, par requetes D3D11 « timestamp » (ENH-004 §5.4).
///
/// Threads :
/// - beginFrame(), endFrame() et release() : thread de rendu de Qt Quick uniquement (seul
///   utilisateur des objets D3D11) ;
/// - takeResults() : n'importe quel thread (en pratique celui de l'interface), sous verrou.
/// L'objet est partage par std::shared_ptr entre RenderPolicy et les connexions du thread de
/// rendu : une image en cours garde sa propre reference, l'objet ne peut donc pas etre detruit
/// sous ses pieds. Les resultats sont lus 2 a 3 images plus tard, sans jamais attendre le GPU.
/// Autre API graphique (rendu logiciel des tests, OpenGL) : la mesure est simplement inactive.
class GpuFrameTimer {
public:
    GpuFrameTimer();
    ~GpuFrameTimer();

    GpuFrameTimer(const GpuFrameTimer&) = delete;
    GpuFrameTimer& operator=(const GpuFrameTimer&) = delete;
    GpuFrameTimer(GpuFrameTimer&&) = delete;
    GpuFrameTimer& operator=(GpuFrameTimer&&) = delete;

    void beginFrame(QQuickWindow* window); ///< QQuickWindow::beforeRendering
    void endFrame(QQuickWindow* window);   ///< QQuickWindow::afterRendering
    void release();                        ///< QQuickWindow::sceneGraphInvalidated

    /// Mesures terminees depuis le dernier appel (ms), dans l'ordre.
    [[nodiscard]] std::vector<double> takeResults();

private:
    void pushResult(double milliseconds);

    struct D3D;                // objets COM (pimpl : d3d11.h reste dans le .cpp)
    std::unique_ptr<D3D> d3d_; // thread de rendu uniquement
    bool unsupported_ = false; // thread de rendu uniquement

    static constexpr std::size_t kMaxPending = 240; // borne si personne ne lit (~4 s a 60 i/s)
    std::mutex resultsMutex_;
    std::vector<double> results_;
};

} // namespace enh::app
