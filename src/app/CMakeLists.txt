# Presentation (ADR-036, ENH-004).
# enh_app_policy : regles de presentation en C++20 pur, sans Qt (compilees et testees meme en CI
#                  sans interface).
# enh_app        : classes Qt exposees au QML + module QML « Enhance.App » (seulement avec Qt).
# enh_design     : module QML « Enhance.Design » (design system : Theme, verre, composants).
add_library(enh_app_policy STATIC policy/glass_quality_governor.cpp policy/app_options.cpp)
enh_target_defaults(enh_app_policy)
target_include_directories(enh_app_policy PUBLIC policy/include)

if(NOT ENH_UI_EFFECTIVE)
  return()
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick ShaderTools)
qt_policy(SET QTP0001 NEW) # ressources QML sous :/qt/qml/
qt_policy(SET QTP0004 NEW)
set(CMAKE_AUTOMOC ON)

# Modules QML ranges sous ${CMAKE_CURRENT_BINARY_DIR}/qml (chemin d'import des tests et outils).
set(ENH_QML_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/qml" CACHE INTERNAL "")

# ---------------------------------------------------------------- Enhance.Design
set(_design_qml Theme.qml ThemedText.qml FocusRing.qml DashedFrame.qml EmptyIllustration.qml
    StatusDot.qml GlassSurface.qml GlassButton.qml)
set(_design_paths)
foreach(_f IN LISTS _design_qml)
  set(_p qml/Enhance/Design/${_f})
  set_source_files_properties(${_p} PROPERTIES QT_RESOURCE_ALIAS ${_f})
  list(APPEND _design_paths ${_p})
endforeach()
set_source_files_properties(qml/Enhance/Design/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_library(enh_design STATIC)
enh_target_defaults(enh_design)
qt_add_qml_module(enh_design
  URI Enhance.Design
  VERSION 1.0
  OUTPUT_DIRECTORY ${ENH_QML_BUILD_DIR}/Enhance/Design
  QML_FILES ${_design_paths}
)
qt_add_shaders(enh_design "enh_glass_shader"
  PREFIX "/qt/qml/Enhance/Design"
  BASE qml/Enhance/Design
  FILES qml/Enhance/Design/shaders/glass.frag
)
target_link_libraries(enh_design PRIVATE Qt6::Quick)

# ---------------------------------------------------------------- Enhance.App (C++ + QML)
set(_app_qml Main.qml AppContent.qml EmptyStateInert.qml SpringDemo.qml AppStatusBar.qml
    GlassTestbed.qml ElevationBanner.qml)
set(_app_paths)
foreach(_f IN LISTS _app_qml)
  set(_p qml/Enhance/App/${_f})
  set_source_files_properties(${_p} PROPERTIES QT_RESOURCE_ALIAS ${_f})
  list(APPEND _app_paths ${_p})
endforeach()

qt_add_library(enh_app STATIC
  app_status.cpp app_status.hpp
  demo_counter_model.cpp demo_counter_model.hpp
  gpu_frame_timer.cpp gpu_frame_timer.hpp
  render_policy.cpp render_policy.hpp
  system_appearance.cpp system_appearance.hpp
  window_controller.cpp window_controller.hpp
)
enh_target_defaults(enh_app)
qt_add_qml_module(enh_app
  URI Enhance.App
  VERSION 1.0
  OUTPUT_DIRECTORY ${ENH_QML_BUILD_DIR}/Enhance/App
  QML_FILES ${_app_paths}
  IMPORTS Enhance.Design
)
target_include_directories(enh_app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(enh_app
  PUBLIC enh_core enh_platform enh_app_policy Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick
  PRIVATE enh_design enh_designplugin)

# Le C++ genere par qmlcachegen (QML compile a l'avance, fichiers .rcc/qmlcache/*.cpp) declenche
# C4702 « code inaccessible » dans les en-tetes de Qt (qmetatype.h, qvariant.h, qjsengine.h) :
# avertissement desactive pour ces seuls fichiers generes, pour que /WX reste utilisable. Les
# sources qmlcachegen n'existent qu'apres la finalisation des cibles par Qt (appel differe) : on
# differe donc aussi, apres Qt.
function(enh_silence_qmlcache_c4702)
  foreach(_target IN ITEMS enh_design enh_app)
    get_target_property(_sources ${_target} SOURCES)
    foreach(_src IN LISTS _sources)
      if(_src MATCHES "qmlcache/.*[.]cpp$")
        set_property(SOURCE "${_src}" TARGET_DIRECTORY ${_target} APPEND PROPERTY COMPILE_OPTIONS /wd4702)
      endif()
    endforeach()
  endforeach()
endfunction()
cmake_language(DEFER CALL enh_silence_qmlcache_c4702)
