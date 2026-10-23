#include "window_controller.hpp"

#include "render_policy.hpp"
#include "system_appearance.hpp"

#include <QByteArray>
#include <QCoreApplication>
#include <QCursor>
#include <QGuiApplication>
#include <QQuickWindow>
#include <QScreen>
#include <QSize>
#include <QTimer>
#include <QWindow>

#include <cmath>
#include <utility>

namespace enh::app {

namespace {

// WId (entier) vers HWND : conversion imposee par l API native de Qt sous Windows.
platform::NativeWindow nativeHandle(const QWindow& window) {
    const WId id = window.winId();
    return reinterpret_cast<platform::NativeWindow>(id); // NOLINT(performance-no-int-to-ptr)
}

} // namespace

WindowController::WindowController(QQuickWindow& window, SystemAppearance& appearance,
                                   RenderPolicy& policy, QObject* parent)
    : QObject(parent), window_(window), appearance_(appearance), policy_(policy) {
    // Habillage reapplique dans la meme iteration que la bascule de Theme (specs §3.2).
    connect(&appearance_, &SystemAppearance::changed, this, &WindowController::applyChrome);
    connect(&policy_, &RenderPolicy::micaUnavailableChanged, this, &WindowController::applyChrome);
    connect(&policy_, &RenderPolicy::captionColorChanged, this, &WindowController::applyChrome);
    // Fond GDI noir (WM_ERASEBKGND) : transparent dans le cadre DWM etendu, donc Mica visible.
    if (auto* app = QCoreApplication::instance()) {
        app->installNativeEventFilter(this);
    }
}

WindowController::~WindowController() {
    if (auto* app = QCoreApplication::instance()) {
        app->removeNativeEventFilter(this);
    }
}

bool WindowController::nativeEventFilter(const QByteArray& eventType, void* message,
                                         qintptr* result) {
    if (eventType != "windows_generic_MSG" || window_.handle() == nullptr) {
        return false;
    }
    const auto hwnd = nativeHandle(window_);
    return platform::handleBackdropMessage(hwnd, message, result);
}

void WindowController::show(std::function<void()> onRevealed) {
    onRevealed_ = std::move(onRevealed);
    window_.create(); // HWND cree : habillage et masquage avant le premier affichage
    placeOnScreen();
    applyChrome();
    cloak_ =
        std::make_unique<platform::WindowCloak>(nativeHandle(window_)); // WId = HWND sous Windows

    // frameSwapped est emis sur le thread de rendu : le devoilement est poste au thread de
    // l'interface. Filet de securite : devoilement force apres kRevealTimeoutMs.
    firstFrame_ = connect(&window_, &QQuickWindow::frameSwapped, this, &WindowController::reveal,
                          Qt::QueuedConnection);
    QTimer::singleShot(kRevealTimeoutMs, this, &WindowController::reveal);
    window_.show();
}

void WindowController::placeOnScreen() {
    QScreen* screen = QGuiApplication::screenAt(QCursor::pos());
    if (screen == nullptr) {
        screen = QGuiApplication::primaryScreen();
    }
    if (screen == nullptr) {
        return;
    }
    window_.setScreen(screen);
    const QRect available = screen->availableGeometry();
    // Taille par defaut (Theme), reduite a 90 % de l ecran, jamais sous la taille minimale.
    const QSize cap(static_cast<int>(std::floor(available.width() * kMaxScreenFraction)),
                    static_cast<int>(std::floor(available.height() * kMaxScreenFraction)));
    const QSize size = window_.size().boundedTo(cap).expandedTo(window_.minimumSize());
    window_.resize(size);
    const int w = size.width();
    const int h = size.height();

    // Centre le cadre complet (barre de titre DWM comprise) sur l'ecran.
    const QMargins frame = window_.frameMargins();
    const int frameW = w + frame.left() + frame.right();
    const int frameH = h + frame.top() + frame.bottom();
    window_.setFramePosition(QPoint(available.x() + ((available.width() - frameW) / 2),
                                    available.y() + ((available.height() - frameH) / 2)));
}

void WindowController::applyChrome() {
    if (window_.handle() == nullptr) {
        return; // fenetre native pas encore creee : show() appliquera
    }
    platform::ChromeState state;
    state.dark = appearance_.dark();
    state.highContrast = appearance_.highContrast();
    state.mica =
        appearance_.transparency() && !appearance_.highContrast() && !policy_.micaUnavailable();
    if (!state.mica && policy_.captionColor().isValid()) {
        state.captionRgb = policy_.captionColor().rgb() & 0xFFFFFFU;
    }
    const auto hwnd = nativeHandle(window_);
    if (!platform::applyWindowChrome(hwnd, state)) {
        policy_.setMicaUnavailable(true); // repli opaque (risque A1) ; reapplique via le signal
    }
}

void WindowController::reveal() {
    // Appele par la premiere image ou par le filet de securite : le premier gagne.
    disconnect(firstFrame_); // sans effet si deja deconnecte
    if (cloak_) {
        cloak_->reveal(); // sans effet si le masquage avait echoue ou est deja leve
    }
    // Le rappel part une seule fois, que le masquage ait reussi ou non.
    if (onRevealed_) {
        std::exchange(onRevealed_, {})();
    }
}

} // namespace enh::app
