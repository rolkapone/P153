# Arborescence : ADR-036. Une cible ne lie que la couche du dessous.
add_subdirectory(common)
add_subdirectory(pipeline)
add_subdirectory(core)
add_subdirectory(platform/win)
add_subdirectory(app) # enh_app_policy toujours ; enh_app / modules QML seulement si ENH_UI_EFFECTIVE

if(ENH_UI_EFFECTIVE)
  # Application de bureau (sous-systeme WIN32 : pas de console).
  add_executable(enhance WIN32 main.cpp enhance.manifest)
  qt_add_resources(enhance "enh_runtime"
    PREFIX "/"
    BASE resources
    FILES resources/qt/etc/qt.conf)
  qt_add_resources(enhance "enh_icons"
    PREFIX "/icons"
    BASE app/icons
    FILES app/icons/enhance-app.svg)
  target_link_libraries(enhance PRIVATE enh_app enh_appplugin enh_designplugin enh_platform
                                        Qt6::Gui Qt6::Quick)
  target_compile_definitions(enhance PRIVATE ENH_WITH_UI=1)
  # SEC-08 : les DLL importees statiquement ne sont cherchees que dans le dossier de
  # l'application et System32 (LOAD_LIBRARY_SEARCH_APPLICATION_DIR | _SYSTEM32).
  target_link_options(enhance PRIVATE /DEPENDENTLOADFLAG:0xA00)
  include(EnhDeployQt)
  enh_deploy_qt_runtime(enhance "${CMAKE_CURRENT_SOURCE_DIR}/app/qml")
else()
  add_executable(enhance main.cpp enhance.manifest)
  target_compile_definitions(enhance PRIVATE ENH_WITH_UI=0)
endif()
enh_target_defaults(enhance)
# UAC : niveau asInvoker fourni par enhance.manifest (une seule declaration trustInfo).
target_link_options(enhance PRIVATE /MANIFESTUAC:NO)
target_link_libraries(enhance PRIVATE enh_core enh_pipeline_impl)
if(ENH_GPU_EFFECTIVE)
  target_compile_definitions(enhance PRIVATE ENH_GPU_BACKEND=1)
else()
  target_compile_definitions(enhance PRIVATE ENH_GPU_BACKEND=0)
endif()
