# Couche Win32/COM sans Qt (ADR-036). Squelette.
add_library(enh_platform STATIC platform_win.cpp)
enh_target_defaults(enh_platform)
target_link_libraries(enh_platform PUBLIC enh_core)
