#include "system_appearance.hpp"

#include <QAbstractNativeEventFilter>
#include <QByteArray>
#include <QColor>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QPalette>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <utility>

namespace enh::app {

// Ecoute les messages Windows qui signalent un changement d'apparence. Qt passe tous les
// messages des fenetres (y compris envoyes par SendMessage) aux filtres natifs.
class SystemAppearance::NativeFilter final : public QAbstractNativeEventFilter {
public:
    explicit NativeFilter(SystemAppearance& owner) : owner_(owner) {}

    bool nativeEventFilter(const QByteArray& eventType, void* message,
                           qintptr* /*result*/) override {
        if (eventType != "windows_generic_MSG" || message == nullptr) {
            return false;
        }
        const auto* msg = static_cast<const MSG*>(message);
        switch (msg->message) {
        case WM_SETTINGCHANGE:
        case WM_THEMECHANGED:
        case WM_SYSCOLORCHANGE:
        case WM_DWMCOLORIZATIONCOLORCHANGED: // couleur d accentuation, ColorPrevalence
            owner_.scheduleRefresh();
            break;
        default:
            break;
        }
        return false; // on observe seulement : le message continue son chemin
    }

private:
    SystemAppearance& owner_;
};

SystemAppearance::SystemAppearance(QObject* parent)
    : SystemAppearance(&platform::readAppearance, parent) {
    filter_ = std::make_unique<NativeFilter>(*this);
    if (auto* app = QCoreApplication::instance()) {
        app->installNativeEventFilter(filter_.get());
    }
}

SystemAppearance::SystemAppearance(Reader reader, QObject* parent)
    : QObject(parent), reader_(std::move(reader)) {
    current_ = reader_ ? reader_() : platform::Appearance{};
    refreshTimer_.setSingleShot(true);
    refreshTimer_.setInterval(kRefreshDelayMs);
    connect(&refreshTimer_, &QTimer::timeout, this, &SystemAppearance::refresh);
    commitTimer_.setSingleShot(true);
    commitTimer_.setInterval(kCommitTimeoutMs);
    connect(&commitTimer_, &QTimer::timeout, this, &SystemAppearance::commitPending);
    applyHighContrastPalette();
}

SystemAppearance::~SystemAppearance() {
    if (filter_) {
        if (auto* app = QCoreApplication::instance()) {
            app->removeNativeEventFilter(filter_.get());
        }
    }
}

void SystemAppearance::scheduleRefresh() {
    refreshTimer_.start(); // relance : une rafale de messages = une seule lecture
}

void SystemAppearance::refresh() {
    if (!reader_) {
        return;
    }
    const platform::Appearance next = reader_();
    const platform::Appearance& reference = pending_ ? *pending_ : current_;
    if (next == reference) {
        return;
    }
    const bool wasPending = pending_.has_value();
    if (next == current_) {
        // Retour a l'etat affiche avant la validation : plus rien a appliquer.
        pending_.reset();
        commitTimer_.stop();
    } else {
        pending_ = next;
        commitTimer_.start();
    }
    if (wasPending != pending_.has_value()) {
        emit changePendingChanged();
    }
}

void SystemAppearance::commitPending() {
    if (!pending_) {
        return;
    }
    commitTimer_.stop();
    current_ = *pending_;
    pending_.reset();
    applyHighContrastPalette();
    emit changed();
    emit changePendingChanged();
}

void SystemAppearance::applyHighContrastPalette() {
    // Theme.qml lit les couleurs systeme par SystemPalette, qui suit la palette de
    // l'application : en contraste eleve on y pose exactement les couleurs de GetSysColor.
    if (QGuiApplication::instance() == nullptr) {
        return;
    }
    if (!current_.highContrast) {
        if (paletteOverridden_) {
            QGuiApplication::setPalette(QPalette());
            paletteOverridden_ = false;
        }
        return;
    }
    const auto rgb = [](std::uint32_t v) { return QColor::fromRgb(static_cast<QRgb>(v)); };
    const platform::SystemColors& c = current_.colors;
    QPalette p;
    p.setColor(QPalette::Window, rgb(c.window));
    p.setColor(QPalette::WindowText, rgb(c.windowText));
    p.setColor(QPalette::Base, rgb(c.window));
    p.setColor(QPalette::Text, rgb(c.windowText));
    p.setColor(QPalette::Button, rgb(c.buttonFace));
    p.setColor(QPalette::ButtonText, rgb(c.buttonText));
    p.setColor(QPalette::Highlight, rgb(c.highlight));
    p.setColor(QPalette::HighlightedText, rgb(c.highlightText));
    p.setColor(QPalette::Disabled, QPalette::WindowText, rgb(c.grayText));
    p.setColor(QPalette::Disabled, QPalette::Text, rgb(c.grayText));
    p.setColor(QPalette::Disabled, QPalette::ButtonText, rgb(c.grayText));
    QGuiApplication::setPalette(p);
    paletteOverridden_ = true;
}

} // namespace enh::app
