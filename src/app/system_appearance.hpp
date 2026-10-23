#pragma once

#include "enh/platform/appearance.hpp"

#include <QObject>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

#include <functional>
#include <memory>
#include <optional>

namespace enh::app {

/// Reglages d'apparence de Windows exposes au QML (branches sur Theme dans Main.qml).
///
/// Changement a chaud (WM_SETTINGCHANGE, WM_THEMECHANGED, WM_SYSCOLORCHANGE) : les nouveaux
/// reglages sont d'abord mis « en attente » (changePending = true) pour que le QML capture
/// l'ancienne image avant la bascule (fondu de 250 ms, ENH-004 specs §3.4), puis appliques par
/// commitPending(). Si personne ne valide, ils s'appliquent seuls apres kCommitTimeout.
class SystemAppearance : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(bool dark READ dark NOTIFY changed FINAL)
    Q_PROPERTY(bool highContrast READ highContrast NOTIFY changed FINAL)
    Q_PROPERTY(bool reduceMotion READ reduceMotion NOTIFY changed FINAL)
    Q_PROPERTY(bool transparency READ transparency NOTIFY changed FINAL)
    Q_PROPERTY(bool changePending READ changePending NOTIFY changePendingChanged FINAL)

public:
    using Reader = std::function<platform::Appearance()>;

    static constexpr int kRefreshDelayMs = 50;   ///< regroupe les rafales de messages Windows
    static constexpr int kCommitTimeoutMs = 500; ///< validation automatique sans reponse du QML

    /// Lit les reglages de Windows et suit leurs changements.
    explicit SystemAppearance(QObject* parent = nullptr);
    /// Source de reglages injectee (tests) ; aucun filtre de messages Windows n'est installe.
    SystemAppearance(Reader reader, QObject* parent);
    ~SystemAppearance() override;

    SystemAppearance(const SystemAppearance&) = delete;
    SystemAppearance& operator=(const SystemAppearance&) = delete;
    SystemAppearance(SystemAppearance&&) = delete;
    SystemAppearance& operator=(SystemAppearance&&) = delete;

    [[nodiscard]] bool dark() const noexcept { return current_.dark; }
    [[nodiscard]] bool highContrast() const noexcept { return current_.highContrast; }
    [[nodiscard]] bool reduceMotion() const noexcept { return current_.reduceMotion; }
    [[nodiscard]] bool transparency() const noexcept { return current_.transparency; }
    [[nodiscard]] bool changePending() const noexcept { return pending_.has_value(); }
    [[nodiscard]] const platform::Appearance& current() const noexcept { return current_; }

    /// Relit les reglages ; s'ils different, les met en attente (changePending).
    Q_INVOKABLE void refresh();
    /// Applique les reglages en attente (sans effet s'il n'y en a pas).
    Q_INVOKABLE void commitPending();

signals:
    void changed();
    void changePendingChanged();

private:
    class NativeFilter;

    void scheduleRefresh();
    void applyHighContrastPalette();

    Reader reader_;
    platform::Appearance current_;
    std::optional<platform::Appearance> pending_;
    QTimer refreshTimer_;
    QTimer commitTimer_;
    std::unique_ptr<NativeFilter> filter_;
    bool paletteOverridden_ = false;
};

} // namespace enh::app
