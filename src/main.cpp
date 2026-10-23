// Racine de composition de enhance.exe.
// Avec l'interface (ENH_WITH_UI) : fenetre Qt Quick (ENH-004). Sans : sortie console (CI).
#include "enh/version.hpp"

#if ENH_WITH_UI
#include "enh/app/app_options.hpp"
#include "enh/platform/process_security.hpp"
#include "enh/platform/window_chrome.hpp"

#include "app_status.hpp"
#include "render_policy.hpp"
#include "system_appearance.hpp"
#include "window_controller.hpp"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QIcon>
#include <QLocale>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QtQml/qqmlextensionplugin.h>

#include <cstdio>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <string_view>
#include <vector>

Q_IMPORT_QML_PLUGIN(Enhance_AppPlugin)
Q_IMPORT_QML_PLUGIN(Enhance_DesignPlugin)

namespace {

QString toQString(const std::filesystem::path& p) {
    return QString::fromStdWString(p.wstring());
}

// SEC-08 : ces variables pourraient faire charger des plugins ou des modules QML hors du dossier
// de l'application ; enhance.exe les ignore (les tests, eux, peuvent s'en servir).
void dropPathOverrides() {
    for (const char* name : {"QT_PLUGIN_PATH", "QT_QPA_PLATFORM_PLUGIN_PATH", "QML_IMPORT_PATH",
                             "QML2_IMPORT_PATH", "QT_QUICK_CONTROLS_STYLE_PATH"}) {
        qunsetenv(name);
    }
}

// --check-modules : liste sur la sortie d'erreur les modules charges hors des dossiers de
// confiance (SEC-08). Codes de sortie : 0 conforme ; 3 module hors confiance ou liste des modules
// illisible ; 5 durcissement du chargement des DLL en echec. Les modules injectes depuis
// Program Files (antivirus...) sont signales sans faire echouer la verification.
int reportUntrustedModules(bool dllSearchHardened) {
    using enh::platform::ModuleOrigin;
    if (!dllSearchHardened) {
        std::fwprintf(stderr, L"ECHEC : SetDefaultDllDirectories / SetDllDirectoryW a echoue.\n");
        return 5;
    }
    const auto roots = enh::platform::currentTrustedModuleRoots();
    const auto injected =
        enh::platform::loadedModulesWithOrigin(roots, ModuleOrigin::ProtectedThirdParty);
    const auto untrusted = enh::platform::loadedModulesWithOrigin(roots, ModuleOrigin::Untrusted);
    if (!injected || !untrusted) {
        std::fwprintf(stderr, L"ECHEC : liste des modules charges incomplete ou illisible.\n");
        return 3;
    }
    for (const auto& path : *injected) {
        std::fwprintf(stderr, L"Note : module injecte par un logiciel installe : %ls\n",
                      path.wstring().c_str());
    }
    for (const auto& path : *untrusted) {
        std::fwprintf(stderr, L"ECHEC : module hors des dossiers de confiance : %ls\n",
                      path.wstring().c_str());
    }
    std::fwprintf(stderr, L"%zu module(s) hors des dossiers de confiance.\n", untrusted->size());
    return untrusted->empty() ? 0 : 3;
}

int runApplication(int argc, char** argv, bool dllSearchHardened) {
    const std::vector<std::string_view> args(argv + (argc > 0 ? 1 : 0), argv + argc);
    const enh::app::AppOptions options = enh::app::parseAppOptions(args);

    // Chemins de Qt fixes avant la creation de l'application (plateforme, modules QML).
    const std::filesystem::path appDir = enh::platform::executableDirectory();
    dropPathOverrides();
    QCoreApplication::setLibraryPaths({toQString(appDir / L"plugins")});
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    // Mica : fenetre a fond transparent (canal alpha) des sa creation (risque A1).
    const bool micaSupported =
        !options.noMica && enh::platform::micaSupportedOnBuild(enh::platform::windowsBuildNumber());
    if (micaSupported) {
        QQuickWindow::setDefaultAlphaBuffer(true);
    }

    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("Enhance"));
    QGuiApplication::setWindowIcon(QIcon(QStringLiteral(":/icons/enhance-app.svg")));
    QLocale::setDefault(QLocale(QLocale::French, QLocale::France));

    QQmlApplicationEngine engine;
    engine.setImportPathList({QStringLiteral("qrc:/qt/qml"), toQString(appDir / L"qml")});
    engine.loadFromModule("Enhance.App", "Main");
    if (engine.rootObjects().isEmpty()) {
        return 1;
    }
    auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().constFirst());
    auto* appearance =
        engine.singletonInstance<enh::app::SystemAppearance*>("Enhance.App", "SystemAppearance");
    auto* policy = engine.singletonInstance<enh::app::RenderPolicy*>("Enhance.App", "RenderPolicy");
    auto* status = engine.singletonInstance<enh::app::AppStatus*>("Enhance.App", "AppStatus");
    if (window == nullptr || appearance == nullptr || policy == nullptr || status == nullptr) {
        return 1;
    }

    policy->configure(options, micaSupported);
    policy->attachWindow(window);
    QObject::connect(status, &enh::app::AppStatus::quitRequested, &app, &QCoreApplication::quit,
                     Qt::QueuedConnection);

    enh::app::WindowController controller(*window, *appearance, *policy);
    if (options.checkModules) {
        controller.show([dllSearchHardened] {
            QCoreApplication::exit(reportUntrustedModules(dllSearchHardened));
        });
        constexpr int kCheckTimeoutMs = 10'000;
        QTimer::singleShot(kCheckTimeoutMs, &app, [] { QCoreApplication::exit(4); });
    } else {
        controller.show();
    }

    const int rc = QGuiApplication::exec();
    // Fermeture : 1) masquer la fenetre arrete son rendu (la boucle threaded attend le thread de
    // rendu) ; 2) seulement ensuite on debranche la mesure GPU ; 3) le moteur detruit la fenetre.
    window->hide();
    policy->detachWindow();
    return rc;
}

} // namespace

int main(int argc, char* argv[]) {
    // SEC-08 : premiere instruction, avant tout chargement dynamique de DLL.
    const bool dllSearchHardened = enh::platform::hardenDllSearchPath();
    if (!dllSearchHardened) {
        std::fprintf(stderr, "Enhance : durcissement du chargement des DLL impossible (SEC-08).\n");
    }
    // Aucune exception ne doit sortir de main (ADR-003) : message clair, code d erreur.
    try {
        return runApplication(argc, argv, dllSearchHardened);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Enhance : erreur inattendue au demarrage (%s).\n", e.what());
    } catch (...) {
        std::fprintf(stderr, "Enhance : erreur inattendue au demarrage.\n");
    }
    return EXIT_FAILURE;
}

#else // sans interface : executable console (CI)

#include <iostream>

int main() {
    try {
        std::cout << "Enhance " << enh::versionString() << " ("
                  << (ENH_GPU_BACKEND != 0 ? "pipeline GPU" : "pipeline simule") << ")\n";
    } catch (...) {
        return 1;
    }
    return 0;
}

#endif
