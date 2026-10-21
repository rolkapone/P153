// Test GPU de fumee : une carte Blackwell (capacite de calcul >= 12.0) est visible par CUDA.
#include <cuda_runtime_api.h>
#include <gtest/gtest.h>

TEST(GpuSmoke, CarteBlackwellVisible) {
    int count = 0;
    ASSERT_EQ(cudaGetDeviceCount(&count), cudaSuccess);
    ASSERT_GE(count, 1);
    cudaDeviceProp prop{};
    ASSERT_EQ(cudaGetDeviceProperties(&prop, 0), cudaSuccess);
    EXPECT_GE(prop.major, 12) << prop.name;
}
