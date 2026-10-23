// Lanceur Qt Quick Test (label ui) : modules Enhance.Design / Enhance.App lies statiquement,
// jetons du design system (tokens.json) exposes aux tests sous le nom « tokens ».
#include <QFile>
#include <QJsonDocument>
#include <QObject>
#include <QQmlContext>
#include <QQmlEngine>
#include <QtQml/qqmlextensionplugin.h>
#include <QtQuickTest/quicktest.h>

Q_IMPORT_QML_PLUGIN(Enhance_AppPlugin)
Q_IMPORT_QML_PLUGIN(Enhance_DesignPlugin)

class Setup : public QObject {
    Q_OBJECT

public slots:
    void qmlEngineAvailable(QQmlEngine* engine) {
        QFile file(QStringLiteral(ENH_TOKENS_JSON));
        QVariant tokens;
        if (file.open(QIODevice::ReadOnly)) {
            tokens = QJsonDocument::fromJson(file.readAll()).toVariant();
        }
        engine->rootContext()->setContextProperty(QStringLiteral("tokens"), tokens);
    }
};

QUICK_TEST_MAIN_WITH_SETUP(enhance_qml, Setup)

#include "main.moc"
