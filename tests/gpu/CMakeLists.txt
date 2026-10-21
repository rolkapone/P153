include(GoogleTest)

add_executable(enh_gpu_tests smoke_device_test.cpp)
enh_target_defaults(enh_gpu_tests)
target_link_libraries(enh_gpu_tests PRIVATE CUDA::cudart_static GTest::gtest GTest::gtest_main)
gtest_discover_tests(enh_gpu_tests PROPERTIES LABELS gpu)
