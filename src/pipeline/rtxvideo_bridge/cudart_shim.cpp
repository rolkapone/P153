// Substituts du runtime CUDA attendus par nvsdk_ngx_s.lib (RTX Video SDK 1.1.0), fournis par l'API
// pilote (docs/gpu/ENH-040.md § 1 et § 7).
//
// Cause (bloquant B4) : la bibliotheque NGX a ete compilee avec les en-tetes CUDA 11.8 et importe
// cudaGetDevice, cudaGetDeviceProperties et cudaGetErrorString (dumpbin /symbols). Avec le cudart
// de CUDA 13, `struct cudaDeviceProp` est plus grande (champs retires, reserved[56] ajoute,
// include/driver_types.h) : cudaGetDeviceProperties ecrit au-dela du tampon 11.8 que NGX a sur sa
// pile -> 0xC0000409 dans NVSDK_NGX_CUDA_Init.
//
// LIMITE CONNUE : cudaGetDeviceProperties n'ecrit que le PREFIXE 11.x de la structure (jusqu'a
// computeMode). Les champs suivants du tampon de NGX ne sont pas remplis ; NGX ne les a pas lus au
// spike, rien ne le garantit pour une autre version du SDK. A revoir a chaque changement de SDK
// (dumpbin /symbols sur la .lib + tests GPU d'ENH-040).
//
// Ne pas inclure cuda_runtime_api.h ici : les declarations seraient incompatibles (volontaire).

#include <cuda.h>

#include <array>
#include <cstddef>
#include <cstring>

namespace {

// Codes cudaError_t utilises (valeurs stables depuis CUDA 10, driver_types.h).
constexpr int kCudaSuccess = 0;
constexpr int kCudaErrorInvalidValue = 1;
constexpr int kCudaErrorInitializationError = 3;
constexpr int kCudaErrorInvalidDevice = 101;

// Prefixe de cudaDeviceProp tel que defini par CUDA 11.x (driver_types.h), champs dans l'ordre.
struct DevicePropV11Prefix {
    std::array<char, 256> name;
    std::array<char, 16> uuid;
    std::array<char, 8> luid;
    unsigned int luidDeviceNodeMask;
    std::size_t totalGlobalMem;
    std::size_t sharedMemPerBlock;
    int regsPerBlock;
    int warpSize;
    std::size_t memPitch;
    int maxThreadsPerBlock;
    std::array<int, 3> maxThreadsDim;
    std::array<int, 3> maxGridSize;
    int clockRate; // retire en CUDA 13
    std::size_t totalConstMem;
    int major;
    int minor;
    std::size_t textureAlignment;
    std::size_t texturePitchAlignment;
    int deviceOverlap; // retire en CUDA 13
    int multiProcessorCount;
    int kernelExecTimeoutEnabled; // retire en CUDA 13
    int integrated;
    int canMapHostMemory;
    int computeMode; // retire en CUDA 13
};

// Disposition figee (x64, CUDA 11.x) : toute derive casserait silencieusement NGX.
static_assert(offsetof(DevicePropV11Prefix, uuid) == 256);
static_assert(offsetof(DevicePropV11Prefix, luid) == 272);
static_assert(offsetof(DevicePropV11Prefix, luidDeviceNodeMask) == 280);
static_assert(offsetof(DevicePropV11Prefix, totalGlobalMem) == 288);
static_assert(offsetof(DevicePropV11Prefix, regsPerBlock) == 304);
static_assert(offsetof(DevicePropV11Prefix, memPitch) == 312);
static_assert(offsetof(DevicePropV11Prefix, maxThreadsPerBlock) == 320);
static_assert(offsetof(DevicePropV11Prefix, maxThreadsDim) == 324);
static_assert(offsetof(DevicePropV11Prefix, maxGridSize) == 336);
static_assert(offsetof(DevicePropV11Prefix, clockRate) == 348);
static_assert(offsetof(DevicePropV11Prefix, totalConstMem) == 352);
static_assert(offsetof(DevicePropV11Prefix, major) == 360);
static_assert(offsetof(DevicePropV11Prefix, minor) == 364);
static_assert(offsetof(DevicePropV11Prefix, textureAlignment) == 368);
static_assert(offsetof(DevicePropV11Prefix, deviceOverlap) == 384);
static_assert(offsetof(DevicePropV11Prefix, computeMode) == 404);
static_assert(sizeof(DevicePropV11Prefix) == 408);

int attr(CUdevice d, CUdevice_attribute a) noexcept {
    int v = 0;
    if (cuDeviceGetAttribute(&v, a, d) != CUDA_SUCCESS) {
        return 0;
    }
    return v;
}

} // namespace

// Noms et signatures imposes par nvsdk_ngx_s.lib (symboles C non decores) : pas de convention
// d'appel a choisir en x64.
// NOLINTBEGIN(readability-identifier-naming)

extern "C" int cudaGetDevice(int* device) noexcept {
    if (device == nullptr) {
        return kCudaErrorInvalidValue;
    }
    CUdevice d{};
    if (cuCtxGetDevice(&d) != CUDA_SUCCESS) {
        *device = 0; // pas de contexte courant : peripherique 0, comme le runtime CUDA
        return kCudaSuccess;
    }
    *device = static_cast<int>(d);
    return kCudaSuccess;
}

extern "C" const char* cudaGetErrorString(int error) noexcept {
    return error == kCudaSuccess ? "cudaSuccess" : "cudaError (enh_rtxvideo_bridge substitute)";
}

extern "C" int cudaGetDeviceProperties(void* out, int device) noexcept {
    if (out == nullptr) {
        return kCudaErrorInvalidValue;
    }
    if (cuInit(0) != CUDA_SUCCESS) {
        return kCudaErrorInitializationError;
    }
    CUdevice d{};
    if (cuDeviceGet(&d, device) != CUDA_SUCCESS) {
        return kCudaErrorInvalidDevice;
    }
    DevicePropV11Prefix p{};
    (void)cuDeviceGetName(p.name.data(), static_cast<int>(p.name.size()), d);
    CUuuid u{};
    if (cuDeviceGetUuid(&u, d) == CUDA_SUCCESS) {
        static_assert(sizeof(u.bytes) == sizeof(p.uuid));
        std::memcpy(p.uuid.data(), u.bytes, p.uuid.size());
    }
    (void)cuDeviceGetLuid(p.luid.data(), &p.luidDeviceNodeMask, d);
    (void)cuDeviceTotalMem(&p.totalGlobalMem, d);
    const auto sz = [d](CUdevice_attribute a) { return static_cast<std::size_t>(attr(d, a)); };
    p.sharedMemPerBlock = sz(CU_DEVICE_ATTRIBUTE_MAX_SHARED_MEMORY_PER_BLOCK);
    p.regsPerBlock = attr(d, CU_DEVICE_ATTRIBUTE_MAX_REGISTERS_PER_BLOCK);
    p.warpSize = attr(d, CU_DEVICE_ATTRIBUTE_WARP_SIZE);
    p.memPitch = sz(CU_DEVICE_ATTRIBUTE_MAX_PITCH);
    p.maxThreadsPerBlock = attr(d, CU_DEVICE_ATTRIBUTE_MAX_THREADS_PER_BLOCK);
    p.maxThreadsDim = {attr(d, CU_DEVICE_ATTRIBUTE_MAX_BLOCK_DIM_X),
                       attr(d, CU_DEVICE_ATTRIBUTE_MAX_BLOCK_DIM_Y),
                       attr(d, CU_DEVICE_ATTRIBUTE_MAX_BLOCK_DIM_Z)};
    p.maxGridSize = {attr(d, CU_DEVICE_ATTRIBUTE_MAX_GRID_DIM_X),
                     attr(d, CU_DEVICE_ATTRIBUTE_MAX_GRID_DIM_Y),
                     attr(d, CU_DEVICE_ATTRIBUTE_MAX_GRID_DIM_Z)};
    p.clockRate = attr(d, CU_DEVICE_ATTRIBUTE_CLOCK_RATE);
    p.totalConstMem = sz(CU_DEVICE_ATTRIBUTE_TOTAL_CONSTANT_MEMORY);
    p.major = attr(d, CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MAJOR);
    p.minor = attr(d, CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MINOR);
    p.textureAlignment = sz(CU_DEVICE_ATTRIBUTE_TEXTURE_ALIGNMENT);
    p.texturePitchAlignment = sz(CU_DEVICE_ATTRIBUTE_TEXTURE_PITCH_ALIGNMENT);
    p.deviceOverlap = attr(d, CU_DEVICE_ATTRIBUTE_GPU_OVERLAP);
    p.multiProcessorCount = attr(d, CU_DEVICE_ATTRIBUTE_MULTIPROCESSOR_COUNT);
    p.kernelExecTimeoutEnabled = attr(d, CU_DEVICE_ATTRIBUTE_KERNEL_EXEC_TIMEOUT);
    p.integrated = attr(d, CU_DEVICE_ATTRIBUTE_INTEGRATED);
    p.canMapHostMemory = attr(d, CU_DEVICE_ATTRIBUTE_CAN_MAP_HOST_MEMORY);
    p.computeMode = attr(d, CU_DEVICE_ATTRIBUTE_COMPUTE_MODE);
    std::memcpy(out, &p, sizeof p); // jamais au-dela du prefixe 11.x
    return kCudaSuccess;
}

// NOLINTEND(readability-identifier-naming)
