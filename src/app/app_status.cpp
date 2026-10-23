#include "app_status.hpp"

#include "enh/platform/process_security.hpp"

namespace enh::app {

AppStatus::AppStatus(QObject* parent) : AppStatus(platform::isProcessElevated(), parent) {}

AppStatus::AppStatus(bool elevated, QObject* parent) : QObject(parent), elevated_(elevated) {}

QString AppStatus::gpuStatusText() const {
    return tr("GeForce RTX 5070 · prête");
}

QString AppStatus::gpuStatusAccessibleName() const {
    return tr("Carte graphique prête");
}

void AppStatus::requestQuit() {
    emit quitRequested();
}

} // namespace enh::app
