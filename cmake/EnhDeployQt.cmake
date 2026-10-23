# Deploiement local du runtime Qt a cote d'un executable (ENH-004, SEC-08).
# Copie dans le dossier de l'exe : DLL Qt et dependances, plugins strictement utiles
# (plateforme Windows, icone SVG), modules QML importes (qmlimportscanner). Aucun plugin reseau,
# aucun plugin de debogage QML (qmltooling). Echoue si un Qt6Network*.dll apparait (controle C2).
# windeployqt n'est pas fourni par le Qt de vcpkg : ce script le remplace pour le developpement ;
# le paquet livrable reste l'affaire de scripts/package.ps1 (ENH-030).

function(enh_deploy_qt_runtime target qml_root)
  get_filename_component(_linker_dir "${CMAKE_LINKER}" DIRECTORY)
  find_program(ENH_DUMPBIN dumpbin HINTS "${_linker_dir}" REQUIRED)
  set(_prefix "${QT6_INSTALL_PREFIX}")
  add_custom_command(TARGET ${target} POST_BUILD
    COMMAND ${CMAKE_COMMAND}
      "-DEXE=$<TARGET_FILE:${target}>"
      "-DQT_BIN=${_prefix}/${QT6_INSTALL_BINS}"
      "-DQT_PLUGINS=${_prefix}/${QT6_INSTALL_PLUGINS}"
      "-DQT_QML=${_prefix}/${QT6_INSTALL_QML}"
      "-DSCANNER=$<TARGET_FILE:Qt6::qmlimportscanner>"
      "-DQML_ROOT=${qml_root}"
      "-DDUMPBIN=${ENH_DUMPBIN}"
      -P "${PROJECT_SOURCE_DIR}/cmake/EnhDeployQtScript.cmake"
    COMMENT "Deploiement du runtime Qt pour ${target}"
    VERBATIM)
endfunction()
