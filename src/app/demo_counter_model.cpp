#include "demo_counter_model.hpp"

namespace enh::app {

DemoCounterModel::DemoCounterModel(QObject* parent) : QObject(parent) {}

void DemoCounterModel::increment() {
    if (counter_.increment()) {
        emit countChanged();
    }
}

} // namespace enh::app
