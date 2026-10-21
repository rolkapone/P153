# Interface Qt Quick (seulement si ENH_UI_EFFECTIVE). Squelette : le developer la remplit (ENH-004).
add_library(enh_app STATIC app_placeholder.cpp)
enh_target_defaults(enh_app)
target_link_libraries(enh_app PUBLIC enh_core Qt6::Quick)
