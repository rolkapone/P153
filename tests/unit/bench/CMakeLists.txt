# ENH-047 : bibliotheque du banc (arguments, BMP, JSON, fichiers, VramMonitor), sans GPU (CI).
add_executable(enh_bench_unit_tests
  args_test.cpp
  bmp_test.cpp
  file_io_test.cpp
  report_test.cpp
  vram_monitor_test.cpp)
enh_target_defaults(enh_bench_unit_tests)
target_compile_definitions(enh_bench_unit_tests PRIVATE
  ENH_BENCH_TEST_DATA="${CMAKE_CURRENT_SOURCE_DIR}/data")
target_link_libraries(enh_bench_unit_tests PRIVATE
  enh_bench_lib nlohmann_json::nlohmann_json GTest::gtest GTest::gtest_main)
gtest_discover_tests(enh_bench_unit_tests PROPERTIES LABELS unit)
