// Noyau minimal : verifie que nvcc compile pour sm_120 avec le compilateur hote MSVC v145.
__global__ void enhProbeKernel(int* out) {
    *out = 1;
}

namespace enh::pipeline {

// Jamais appelee par le produit ; sert seulement de preuve de compilation.
void enhProbeLaunch(int* deviceOut) {
    enhProbeKernel<<<1, 1>>>(deviceOut);
}

}  // namespace enh::pipeline
