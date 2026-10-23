# Tests Qt de la presentation (label ui) : plateforme offscreen, rendu logiciel (ADR-028).
find_package(Qt6 REQUIRED COMPONENTS Test)

add_executable(enh_presentation_tests tst_presentation.cpp)
set_target_properties(enh_presentation_tests PROPERTIES AUTOMOC ON)
enh_target_defaults(enh_presentation_tests)
target_link_libraries(enh_presentation_tests PRIVATE enh_app Qt6::Test)
add_test(NAME enh_presentation_tests COMMAND enh_presentation_tests)
enh_set_qt_test_environment(enh_presentation_tests)

# SEC-08 : enhance.exe deploye ne charge aucun module hors de son dossier, de System32 ou de
# WinSxS (--check-modules quitte apres la premiere image ; code 3 sinon). Seulement si le runtime
# Qt est deploye (Qt de vcpkg, ENH_DEPLOY_QT). Plateforme Windows imposee : le dossier deploye ne
# contient volontairement pas le plugin offscreen (le job CI « ui » fixe offscreen pour tous).
if(ENH_DEPLOY_QT_EFFECTIVE)
  add_test(NAME enhance_loaded_modules COMMAND enhance --check-modules)
  set_tests_properties(enhance_loaded_modules PROPERTIES
    LABELS ui
    TIMEOUT 30
    ENVIRONMENT "QT_QPA_PLATFORM=windows")
endif()
