#include "gpu_frame_timer.hpp"

#include <QQuickWindow>
#include <QSGRendererInterface>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <d3d11.h>
#include <wrl/client.h>

#include <array>
#include <cstddef>
#include <utility>

namespace enh::app {

using Microsoft::WRL::ComPtr; // pointeur intelligent COM : Release() automatique (RAII)

namespace {

constexpr std::size_t kSlots = 4; // images en vol : lecture 2 a 3 images plus tard

struct Slot {
    ComPtr<ID3D11Query> disjoint;
    ComPtr<ID3D11Query> start;
    ComPtr<ID3D11Query> stop;
    bool pending = false;
};

} // namespace

struct GpuFrameTimer::D3D {
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    std::array<Slot, kSlots> queries{};
    std::size_t next = 0;
    Slot* open = nullptr; // requete commencee dans beginFrame, a terminer dans endFrame
};

GpuFrameTimer::GpuFrameTimer() = default;

GpuFrameTimer::~GpuFrameTimer() = default;

void GpuFrameTimer::pushResult(double milliseconds) {
    const std::scoped_lock lock(resultsMutex_);
    if (results_.size() >= kMaxPending) {
        results_.erase(results_.begin()); // personne ne lit : on garde les plus recentes
    }
    results_.push_back(milliseconds);
}

std::vector<double> GpuFrameTimer::takeResults() {
    const std::scoped_lock lock(resultsMutex_);
    return std::exchange(results_, {});
}

void GpuFrameTimer::beginFrame(QQuickWindow* window) {
    if (unsupported_ || window == nullptr) {
        return;
    }
    if (!d3d_) {
        const QSGRendererInterface* ri = window->rendererInterface();
        if (ri == nullptr || ri->graphicsApi() != QSGRendererInterface::Direct3D11) {
            unsupported_ = true;
            return;
        }
        auto* device = static_cast<ID3D11Device*>(
            ri->getResource(window, QSGRendererInterface::DeviceResource));
        auto* context = static_cast<ID3D11DeviceContext*>(
            ri->getResource(window, QSGRendererInterface::DeviceContextResource));
        if (device == nullptr || context == nullptr) {
            unsupported_ = true;
            return;
        }
        d3d_ = std::make_unique<D3D>();
        d3d_->device = device; // ComPtr : AddRef, Release a la destruction
        d3d_->context = context;
        for (Slot& s : d3d_->queries) {
            const D3D11_QUERY_DESC disjointDesc{.Query = D3D11_QUERY_TIMESTAMP_DISJOINT,
                                                .MiscFlags = 0};
            const D3D11_QUERY_DESC stampDesc{.Query = D3D11_QUERY_TIMESTAMP, .MiscFlags = 0};
            if (FAILED(device->CreateQuery(&disjointDesc, &s.disjoint)) ||
                FAILED(device->CreateQuery(&stampDesc, &s.start)) ||
                FAILED(device->CreateQuery(&stampDesc, &s.stop))) {
                d3d_.reset();
                unsupported_ = true;
                return;
            }
        }
    }

    // 1. Recolte des mesures terminees, sans attendre (DONOTFLUSH).
    for (Slot& s : d3d_->queries) {
        if (!s.pending) {
            continue;
        }
        D3D11_QUERY_DATA_TIMESTAMP_DISJOINT dj{};
        UINT64 t0 = 0;
        UINT64 t1 = 0;
        constexpr UINT flags = D3D11_ASYNC_GETDATA_DONOTFLUSH;
        if (d3d_->context->GetData(s.disjoint.Get(), &dj, sizeof(dj), flags) != S_OK ||
            d3d_->context->GetData(s.start.Get(), &t0, sizeof(t0), flags) != S_OK ||
            d3d_->context->GetData(s.stop.Get(), &t1, sizeof(t1), flags) != S_OK) {
            continue; // pas encore pret : on reessaiera a l'image suivante
        }
        s.pending = false;
        if (dj.Disjoint == FALSE && dj.Frequency > 0 && t1 >= t0) {
            pushResult(static_cast<double>(t1 - t0) * 1000.0 / static_cast<double>(dj.Frequency));
        }
    }

    // 2. Nouvelle mesure si un emplacement est libre (sinon cette image n'est pas mesuree).
    Slot& slot = d3d_->queries[d3d_->next];
    if (slot.pending) {
        d3d_->open = nullptr;
        return;
    }
    // Appele dans la passe principale (beforeRenderPassRecording / afterRenderPassRecording) :
    // la mesure couvre le dessin de la scene, et non l'attente de l'image libre du swapchain
    // (mode flip) ni le delai de soumission, qui faisaient suivre la valeur au vsync quand elle
    // etait prise dans beforeRendering / afterRendering. beginExternalCommands vide les commandes
    // deja enregistrees par Qt : nos requetes sont ordonnees avec celles de la passe.
    window->beginExternalCommands();
    d3d_->context->Begin(slot.disjoint.Get());
    d3d_->context->End(slot.start.Get());
    window->endExternalCommands();
    d3d_->open = &slot;
}

void GpuFrameTimer::endFrame(QQuickWindow* window) {
    if (!d3d_ || d3d_->open == nullptr || window == nullptr) {
        return;
    }
    window->beginExternalCommands();
    d3d_->context->End(d3d_->open->stop.Get());
    d3d_->context->End(d3d_->open->disjoint.Get());
    window->endExternalCommands();
    d3d_->open->pending = true;
    d3d_->open = nullptr;
    d3d_->next = (d3d_->next + 1) % kSlots;
}

void GpuFrameTimer::release() {
    d3d_.reset(); // Release() de toutes les requetes et du peripherique
    unsupported_ = false;
}

} // namespace enh::app
