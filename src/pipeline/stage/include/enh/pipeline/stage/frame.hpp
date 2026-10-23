#pragma once

// Images en transit entre etages (docs/gpu/architecture-pipeline.md § 3.2). Interfaces INTERNES au
// pipeline : l'application n'inclut pas les en-tetes de enh/pipeline/stage/.
// Aucun type CUDA ni FFmpeg : les faux (CI) et les vrais etages partagent ces types.

#include "enh/pipeline/error.hpp"
#include "enh/pipeline/types.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <stop_token>

namespace enh::pipeline {

/// Formats d'image en sortie du decodage (ADR-042) : NV12 (8 bits) ou P010 (10 bits, aussi pour
/// les sources 12 bits : bits de poids fort conserves).
enum class PixelFormat : std::uint8_t { Nv12, P010 };

/// Ou vivent les pixels : memoire CPU (faux, decodage logiciel sans GPU) ou memoire GPU.
enum class MemoryKind : std::uint8_t { Host, Device };

enum class ColorMatrix : std::uint8_t { Unspecified, Bt601, Bt709, Bt2020Ncl, Other };
enum class ColorPrimaries : std::uint8_t { Unspecified, Bt601, Bt709, Bt2020, Other };
enum class ColorTransfer : std::uint8_t { Unspecified, Bt709, Pq, Hlg, Other };
enum class ColorRange : std::uint8_t { Unspecified, Limited, Full };

/// Description couleur lue dans le flux ; les regles par defaut sont appliquees plus loin
/// (ADR-042).
struct ColorInfo {
    ColorMatrix matrix = ColorMatrix::Unspecified;
    ColorPrimaries primaries = ColorPrimaries::Unspecified;
    ColorTransfer transfer = ColorTransfer::Unspecified;
    ColorRange range = ColorRange::Unspecified;
    friend constexpr bool operator==(const ColorInfo&, const ColorInfo&) = default;
};

struct FrameDesc {
    Size size_px;
    PixelFormat format = PixelFormat::Nv12;
    ColorInfo color;
    friend constexpr bool operator==(const FrameDesc&, const FrameDesc&) = default;
};

/// Un plan d'image. `address` est un pointeur CPU (Host) ou un CUdeviceptr (Device).
struct PlaneView {
    std::uint64_t address = 0;
    std::size_t pitch_bytes = 0; // distance entre deux lignes
    std::size_t width_bytes = 0; // octets utiles par ligne
    std::size_t rows = 0;
};

inline constexpr std::size_t kMaxPlanes = 2; // NV12 / P010 : Y puis UV entrelaces

/// Memoire d'une image. Classe RAII : le destructeur rend la memoire a son pool (aucune
/// allocation dans la boucle). Implementations : HostFrameBuffer (CPU), DeviceFrameBuffer (CUDA).
class FrameBuffer {
public:
    FrameBuffer() = default;
    FrameBuffer(const FrameBuffer&) = delete;
    FrameBuffer& operator=(const FrameBuffer&) = delete;
    FrameBuffer(FrameBuffer&&) = delete;
    FrameBuffer& operator=(FrameBuffer&&) = delete;
    virtual ~FrameBuffer() = default;

    [[nodiscard]] virtual MemoryKind memory() const noexcept = 0;
    [[nodiscard]] virtual std::span<const PlaneView> planes() const noexcept = 0;

    /// Evenement CUDA (cudaEvent_t) apres lequel les pixels sont valides ; nullptr en memoire CPU.
    /// Le consommateur fait cudaStreamWaitEvent (attente cote GPU, ADR-045).
    [[nodiscard]] virtual void* readyEvent() const noexcept { return nullptr; }

    /// Le consommateur declare son dernier usage sur son flux CUDA (cudaStream_t) : le pool
    /// attendra cet instant cote GPU avant de reutiliser la surface. Sans appel, l'attente porte
    /// sur le flux producteur.
    virtual void recordLastUse(void* /*cuda_stream*/) noexcept {}
};

using FrameBufferPtr = std::unique_ptr<FrameBuffer>;

enum class FrameFlags : std::uint8_t { None = 0, SceneCut = 1, Generated = 2, Duplicated = 4 };

inline constexpr std::int64_t kNoPts = INT64_MIN;

/// Image en transit : deplacable, non copiable ; la surface revient au pool a la destruction.
struct VideoFrame {
    FrameDesc desc;
    std::int64_t pts = kNoPts; // dans time_base
    Rational time_base;
    std::int64_t index = 0; // numero d'image (ordre de sortie du decodeur)
    FrameFlags flags = FrameFlags::None;
    FrameBufferPtr buffer;
};

/// Etage suivant (une file bornee en pratique). push bloque si plein (back-pressure) et rend
/// Cancelled si st est arrete pendant l'attente.
class IFrameSink {
public:
    IFrameSink() = default;
    IFrameSink(const IFrameSink&) = delete;
    IFrameSink& operator=(const IFrameSink&) = delete;
    IFrameSink(IFrameSink&&) = delete;
    IFrameSink& operator=(IFrameSink&&) = delete;
    virtual ~IFrameSink() = default;

    virtual Result<void> push(VideoFrame&& frame, std::stop_token st) = 0;
};

} // namespace enh::pipeline
