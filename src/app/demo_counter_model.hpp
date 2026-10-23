#pragma once

#include "enh/demo_counter.hpp"

#include <QObject>
#include <QtQml/qqmlregistration.h>

namespace enh::app {

/// Adaptateur QML du compteur de demonstration (ENH-004, provisoire, retire avec ENH-016).
/// La regle (incrementer, saturer) est dans enh::DemoCounter (enh_core) ; cette classe ne fait
/// que l'exposer : Q_PROPERTY lue par le QML, Q_INVOKABLE appelee par le QML.
class DemoCounterModel : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(DemoCounter)
    QML_SINGLETON
    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)

public:
    explicit DemoCounterModel(QObject* parent = nullptr);

    [[nodiscard]] int count() const noexcept { return counter_.count(); }

    /// Action « Tester le ressort ».
    Q_INVOKABLE void increment();

signals:
    void countChanged();

private:
    DemoCounter counter_;
};

} // namespace enh::app
