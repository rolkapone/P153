include(GoogleTest)

add_executable(enh_unit_tests core/version_test.cpp)
enh_target_defaults(enh_unit_tests)
target_link_libraries(enh_unit_tests PRIVATE enh_core GTest::gtest GTest::gtest_main)
gtest_discover_tests(enh_unit_tests PROPERTIES LABELS unit)
