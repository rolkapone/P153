#pragma once

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace enh::app {

/// Etat global affiche par la fenetre : barre d'etat, bandeau administrateur, sortie.
class AppStatus : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    /// Texte de la barre d'etat. ENH-004 : texte fixe (decision PO du 2026-10-21) ; la detection
    /// reelle de la carte arrive avec ENH-031.
    Q_PROPERTY(QString gpuStatusText READ gpuStatusText CONSTANT FINAL)
    /// Nom accessible de la pastille d'etat.
    Q_PROPERTY(QString gpuStatusAccessibleName READ gpuStatusAccessibleName CONSTANT FINAL)
    /// SEC-20 : processus lance avec un jeton eleve (« en tant qu'administrateur »).
    Q_PROPERTY(bool elevated READ elevated CONSTANT FINAL)

public:
    explicit AppStatus(QObject* parent = nullptr);
    /// Constructeur de test : etat d'elevation impose.
    AppStatus(bool elevated, QObject* parent);

    [[nodiscard]] QString gpuStatusText() const;
    [[nodiscard]] QString gpuStatusAccessibleName() const;
    [[nodiscard]] bool elevated() const noexcept { return elevated_; }

    /// Ctrl+Q. ENH-004 : aucun traitement possible, donc sortie immediate, sans feuille.
    Q_INVOKABLE void requestQuit();

signals:
    void quitRequested();

private:
    bool elevated_ = false;
};

} // namespace enh::app
