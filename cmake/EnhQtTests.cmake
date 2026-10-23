# Environnement des tests Qt (label ui) : plateforme offscreen et rendu logiciel (ADR-028), Qt
# trouve par PATH / QT_PLUGIN_PATH / QML_IMPORT_PATH (les tests ne sont pas deployes).
function(enh_set_qt_test_environment test_name)
  set(_prefix "${QT6_INSTALL_PREFIX}")
  # Plateforme offscreen : polices lues dans le dossier des polices de Windows.
  file(TO_CMAKE_PATH "$ENV{SystemRoot}/Fonts" _fonts)
  set_tests_properties(${test_name} PROPERTIES
    LABELS ui
    ENVIRONMENT "QT_QPA_FONTDIR=${_fonts};QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software;QT_PLUGIN_PATH=${_prefix}/${QT6_INSTALL_PLUGINS};QML_IMPORT_PATH=${_prefix}/${QT6_INSTALL_QML}"
    ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${_prefix}/${QT6_INSTALL_BINS}")
endfunction()
