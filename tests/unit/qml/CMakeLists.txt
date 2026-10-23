# Tests Qt Quick Test (label ui) : Theme compare a tokens.json, squelette ENH-004 (textes,
# clavier, replis). Rendu logiciel offscreen : aucun GPU (ADR-028).
find_package(Qt6 REQUIRED COMPONENTS QuickTest)

add_executable(enh_qml_tests main.cpp)
set_target_properties(enh_qml_tests PROPERTIES AUTOMOC ON)
enh_target_defaults(enh_qml_tests)
target_link_libraries(enh_qml_tests PRIVATE enh_app enh_appplugin enh_designplugin Qt6::QuickTest)
target_compile_definitions(enh_qml_tests PRIVATE
  ENH_TOKENS_JSON="${PROJECT_SOURCE_DIR}/docs/design/ui/tokens.json"
  QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME enh_qml_tests COMMAND enh_qml_tests -input "${CMAKE_CURRENT_SOURCE_DIR}")
enh_set_qt_test_environment(enh_qml_tests)
