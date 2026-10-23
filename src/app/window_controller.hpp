#pragma once

#include "enh/platform/window_chrome.hpp"

#include <QAbstractNativeEventFilter>
#include <QObject>

#include <functional>
#include <memory>

class QQuickWindow;

namespace enh::app {

class RenderPolicy;
class SystemAppearance;

/// Fenetre principale cote C++ : taille et position initiales, habillage DWM (Mica, barre de
/// titre) tenu a jour, ouverture sans flash (fenetre masquee jusqu'a sa premiere image).
class WindowController : public QObject, private QAbstractNativeEventFilter {
    Q_OBJECT

public:
    /// Part de l'ecran disponible au-dela de laquelle la taille par defaut est reduite.
    static constexpr double kMaxScreenFraction = 0.9;
    /// Delai maximal avant de devoiler la fenetre si aucune image n'est signalee.
    static constexpr int kRevealTimeoutMs = 1500;

    WindowController(QQuickWindow& window, SystemAppearance& appearance, RenderPolicy& policy,
                     QObject* parent = nullptr);
    ~WindowController() override;

    WindowController(const WindowController&) = delete;
    WindowController& operator=(const WindowController&) = delete;
    WindowController(WindowController&&) = delete;
    WindowController& operator=(WindowController&&) = delete;

    /// Place la fenetre, applique l'habillage, la masque puis l'affiche. `onRevealed` est appele
    /// une fois la premiere image affichee (mesures de demarrage, --check-modules).
    void show(std::function<void()> onRevealed = {});

private:
    void placeOnScreen();
    void applyChrome();
    void reveal();
    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;

    QQuickWindow& window_;
    SystemAppearance& appearance_;
    RenderPolicy& policy_;
    std::unique_ptr<platform::WindowCloak> cloak_;
    std::function<void()> onRevealed_;
    QMetaObject::Connection firstFrame_;
};

} // namespace enh::app
