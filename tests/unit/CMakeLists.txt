include(GoogleTest)

add_executable(enh_unit_tests
  core/version_test.cpp
  core/demo_counter_test.cpp
  app/glass_quality_governor_test.cpp
  app/app_options_test.cpp
  platform/appearance_test.cpp
  platform/process_security_test.cpp)
enh_target_defaults(enh_unit_tests)
target_link_libraries(enh_unit_tests PRIVATE enh_core enh_app_policy enh_platform
                                             GTest::gtest GTest::gtest_main)
gtest_discover_tests(enh_unit_tests PROPERTIES LABELS unit)

# Tests de la presentation Qt (label ui) : seulement si Qt est present (ADR-028).
if(ENH_UI_EFFECTIVE)
  include(EnhQtTests)
  add_subdirectory(app)
  add_subdirectory(qml)
endif()
