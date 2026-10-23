// Tests Qt de la couche de presentation (label ui, sans GPU : plateforme offscreen).
#include "app_status.hpp"
#include "demo_counter_model.hpp"
#include "render_policy.hpp"
#include "system_appearance.hpp"

#include <QGuiApplication>
#include <QPalette>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QTest>

#include <memory>

using enh::app::AppStatus;
using enh::app::DemoCounterModel;
using enh::app::RenderPolicy;
using enh::app::SystemAppearance;
using enh::platform::Appearance;

class PresentationTest : public QObject {
    Q_OBJECT

private slots:
    // La palette de l'application est globale : chaque test repart de la palette d'origine.
    void initTestCase() { originalPalette_ = QGuiApplication::palette(); }
    void cleanup() { QGuiApplication::setPalette(originalPalette_); }

    // ------------------------------------------------------------- DemoCounter
    void compteurExposeUnePropriete() {
        DemoCounterModel model;
        QCOMPARE(model.property("count").toInt(), 0);
        QSignalSpy spy(&model, &DemoCounterModel::countChanged);
        QVERIFY(QMetaObject::invokeMethod(&model, "increment")); // appel comme depuis le QML
        QCOMPARE(spy.count(), 1);
        QCOMPARE(model.property("count").toInt(), 1);
    }

    // ------------------------------------------------------------- AppStatus
    void barreEtatTexteExact() {
        const AppStatus status(false, nullptr);
        QCOMPARE(status.gpuStatusText(), QStringLiteral("GeForce RTX 5070 · prête"));
        QCOMPARE(status.gpuStatusAccessibleName(), QStringLiteral("Carte graphique prête"));
        QVERIFY(!status.elevated());
        QVERIFY(AppStatus(true, nullptr).elevated());
    }

    void quitterEmetUnSignal() {
        AppStatus status(false, nullptr);
        QSignalSpy spy(&status, &AppStatus::quitRequested);
        status.requestQuit();
        QCOMPARE(spy.count(), 1);
    }

    // ------------------------------------------------------------- SystemAppearance
    void apparenceInitialeLueALaConstruction() {
        Appearance a;
        a.dark = false;
        a.reduceMotion = true;
        const SystemAppearance sa([a] { return a; }, nullptr);
        QVERIFY(!sa.dark());
        QVERIFY(sa.reduceMotion());
        QVERIFY(sa.transparency());
        QVERIFY(!sa.changePending());
    }

    void changementMisEnAttentePuisValide() {
        auto source = std::make_shared<Appearance>();
        SystemAppearance sa([source] { return *source; }, nullptr);
        QSignalSpy changed(&sa, &SystemAppearance::changed);
        QSignalSpy pending(&sa, &SystemAppearance::changePendingChanged);

        source->dark = false;
        sa.refresh();
        QVERIFY(sa.changePending());
        QVERIFY(sa.dark()); // pas encore applique : le QML capture d'abord l'image
        QCOMPARE(changed.count(), 0);
        QCOMPARE(pending.count(), 1);

        sa.commitPending();
        QVERIFY(!sa.dark());
        QVERIFY(!sa.changePending());
        QCOMPARE(changed.count(), 1);
    }

    void retourALEtatAfficheAnnuleLAttente() {
        auto source = std::make_shared<Appearance>();
        SystemAppearance sa([source] { return *source; }, nullptr);
        source->transparency = false;
        sa.refresh();
        QVERIFY(sa.changePending());
        source->transparency = true;
        sa.refresh();
        QVERIFY(!sa.changePending());
        sa.commitPending(); // sans effet
        QVERIFY(sa.transparency());
    }

    void validationAutomatiqueSansReponseDuQml() {
        auto source = std::make_shared<Appearance>();
        SystemAppearance sa([source] { return *source; }, nullptr);
        source->highContrast = true;
        sa.refresh();
        QVERIFY(sa.changePending());
        QTRY_VERIFY_WITH_TIMEOUT(!sa.changePending(), SystemAppearance::kCommitTimeoutMs * 4);
        QVERIFY(sa.highContrast());
    }

    void contrasteEleveAppliqueLesCouleursSysteme() {
        Appearance a;
        a.highContrast = true;
        a.colors.window = 0x000000U;
        a.colors.windowText = 0xFFFF00U;
        a.colors.highlight = 0x1AEBFFU;
        const SystemAppearance sa([a] { return a; }, nullptr);
        const QPalette p = QGuiApplication::palette();
        QCOMPARE(p.color(QPalette::Window), QColor(0, 0, 0));
        QCOMPARE(p.color(QPalette::WindowText), QColor(0xFF, 0xFF, 0x00));
        QCOMPARE(p.color(QPalette::Highlight), QColor(0x1A, 0xEB, 0xFF));
    }

    // ------------------------------------------------------------- RenderPolicy
    void verreCompletParDefaut() {
        const RenderPolicy policy;
        QCOMPARE(policy.glassQuality(), 2);
        QVERIFY(!policy.micaUnavailable());
        QVERIFY(!policy.glassTestbed());
    }

    void optionsAppliquees() {
        RenderPolicy policy;
        QSignalSpy quality(&policy, &RenderPolicy::glassQualityChanged);
        QSignalSpy mica(&policy, &RenderPolicy::micaUnavailableChanged);
        enh::app::AppOptions options;
        options.forcedGlassQuality = enh::app::GlassQuality::Opaque;
        options.glassTestbed = true;
        policy.configure(options, /*micaSupported=*/false);
        QCOMPARE(policy.glassQuality(), 0);
        QVERIFY(policy.glassTestbed());
        QVERIFY(policy.micaUnavailable());
        QCOMPARE(quality.count(), 1);
        QCOMPARE(mica.count(), 1);
    }

    void degradationSeulementPendantUnTraitement() {
        RenderPolicy policy;
        for (int i = 0; i < 60; ++i) {
            policy.reportGpuFrameTime(9.0);
        }
        QCOMPARE(policy.glassQuality(), 2); // au repos : toujours complet
        policy.setProcessing(true);
        for (int i = 0; i < 30; ++i) {
            policy.reportGpuFrameTime(9.0);
        }
        QCOMPARE(policy.glassQuality(), 0);
        policy.setProcessing(false);
        QCOMPARE(policy.glassQuality(), 2);
    }

    void qualiteForceeIgnoreLeGouverneur() {
        RenderPolicy policy;
        enh::app::AppOptions options;
        options.forcedGlassQuality = enh::app::GlassQuality::Lite;
        policy.configure(options, true);
        policy.setProcessing(true);
        for (int i = 0; i < 30; ++i) {
            policy.reportGpuFrameTime(20.0);
        }
        QCOMPARE(policy.glassQuality(), 1);
    }

    void couleurDeLegende() {
        RenderPolicy policy;
        QSignalSpy spy(&policy, &RenderPolicy::captionColorChanged);
        policy.setCaptionColor(QColor(0x1B, 0x1B, 0x1E));
        policy.setCaptionColor(QColor(0x1B, 0x1B, 0x1E));
        QCOMPARE(spy.count(), 1);
    }

    // B1 (revue cycle 1) : debrancher ou detruire la politique pendant que la fenetre rend des
    // images en continu ne doit rien casser (minuteur partage par shared_ptr).
    // Attention : en CI (offscreen + rendu logiciel) la boucle de rendu n'est pas threaded et
    // la mesure D3D11 est inactive ; le test n'y verifie que la duree de vie des connexions. La
    // preuve de l'absence de course n'est apportee qu'en local, plateforme windows + D3D11 :
    //   QT_QPA_PLATFORM=windows enh_presentation_tests fermeturePendantLeRendu
    void fermeturePendantLeRendu() {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            Window {
                width: 200; height: 120; visible: true
                Rectangle {
                    width: 40; height: 40; color: "red"
                    RotationAnimator on rotation { from: 0; to: 360; duration: 300; loops: Animation.Infinite }
                }
            })",
                          QUrl());
        for (int round = 0; round < 10; ++round) {
            std::unique_ptr<QObject> root(component.create());
            auto* window = qobject_cast<QQuickWindow*>(root.get());
            QVERIFY2(window != nullptr, qPrintable(component.errorString()));
            QVERIFY(QTest::qWaitForWindowExposed(window));
            {
                auto policy = std::make_unique<RenderPolicy>();
                policy->attachWindow(window);
                QTest::qWait(20);
                if (round % 2 == 0) {
                    policy->detachWindow(); // debranchement explicite, fenetre visible
                }
            } // destruction de la politique pendant le rendu
            QTest::qWait(20);
            window->hide();
        }
    }

private:
    QPalette originalPalette_;
};

QTEST_MAIN(PresentationTest)
#include "tst_presentation.moc"
