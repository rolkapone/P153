#pragma once

#include "enh/app/app_options.hpp"
#include "enh/app/glass_quality_governor.hpp"

#include <QColor>
#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QtQml/qqmlregistration.h>

#include <memory>
#include <optional>

class QQuickWindow;

namespace enh::app {

class GpuFrameTimer;

/// Politique de rendu de la fenetre : qualite du verre (budget GPU de l'interface), repli
/// opaque du Mica (risque A1), banc d'essai du verre, couleur de la barre de titre.
/// C'est de la logique de presentation, pas de la logique metier (design-system §10).
class RenderPolicy : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    /// 2 complete, 1 allegee, 0 opaque (-> Theme.glassQualityRequested).
    Q_PROPERTY(int glassQuality READ glassQuality NOTIFY glassQualityChanged FINAL)
    /// Mica indisponible (Windows < 22H2, refus de DWM, --no-mica) : tout passe en opaque.
    Q_PROPERTY(bool micaUnavailable READ micaUnavailable NOTIFY micaUnavailableChanged FINAL)
    /// --glass-testbed : motif de test sous la capsule (QA, controle design).
    Q_PROPERTY(bool glassTestbed READ glassTestbed NOTIFY glassTestbedChanged FINAL)
    /// Couleur de fond (Theme.color.bgWindow), posee par Main.qml : barre de titre en opaque.
    Q_PROPERTY(QColor captionColor READ captionColor WRITE setCaptionColor NOTIFY
                   captionColorChanged FINAL)
    /// Temps GPU moyen de rendu de l'interface sur la derniere seconde (ms ; 0 si non mesure).
    Q_PROPERTY(double uiGpuMs READ uiGpuMs NOTIFY uiGpuMsChanged FINAL)

public:
    explicit RenderPolicy(QObject* parent = nullptr);
    ~RenderPolicy() override;

    RenderPolicy(const RenderPolicy&) = delete;
    RenderPolicy& operator=(const RenderPolicy&) = delete;
    RenderPolicy(RenderPolicy&&) = delete;
    RenderPolicy& operator=(RenderPolicy&&) = delete;

    [[nodiscard]] int glassQuality() const noexcept;
    [[nodiscard]] bool micaUnavailable() const noexcept { return micaUnavailable_; }
    [[nodiscard]] bool glassTestbed() const noexcept { return options_.glassTestbed; }
    [[nodiscard]] QColor captionColor() const { return captionColor_; }
    [[nodiscard]] double uiGpuMs() const noexcept { return uiGpuMs_; }

    void setCaptionColor(const QColor& color);

    /// Options de ligne de commande et support du Mica par le systeme.
    void configure(const AppOptions& options, bool micaSupported);
    /// DWM a refuse le Mica : repli opaque definitif pour la session.
    void setMicaUnavailable(bool unavailable);
    /// Traitement GPU en cours (ENH-010+) : active la degradation automatique du verre.
    void setProcessing(bool processing);

    /// Branche la mesure du temps GPU sur la fenetre (Direct3D 11 seulement ; sinon sans effet).
    void attachWindow(QQuickWindow* window);
    /// Debranche la mesure (avant la destruction de la fenetre).
    void detachWindow();

    /// Mesure d'une image (thread de l'interface).
    void reportGpuFrameTime(double milliseconds);

signals:
    void glassQualityChanged();
    void micaUnavailableChanged();
    void glassTestbedChanged();
    void captionColorChanged();
    void uiGpuMsChanged();

private:
    void setGovernedQuality(GlassQuality quality);

    AppOptions options_;
    bool micaUnavailable_ = false;
    bool processing_ = false;
    QColor captionColor_;
    GlassQualityGovernor governor_;
    GlassQuality governed_ = GlassQuality::Full;
    double uiGpuMs_ = 0.0;
    double windowSumMs_ = 0.0;
    int windowFrames_ = 0;
    GlassQualityGovernor::Clock::time_point windowStart_{};

    QPointer<QQuickWindow> window_;
    std::unique_ptr<GpuFrameTimer> timer_;
    QMetaObject::Connection beforeConnection_;
    QMetaObject::Connection afterConnection_;
    QMetaObject::Connection invalidatedConnection_;
};

} // namespace enh::app
