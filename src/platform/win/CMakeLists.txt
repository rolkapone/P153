# Couche Win32/COM sans Qt (ADR-036) : apparence Windows, habillage DWM, durcissement du processus.
add_library(enh_platform STATIC appearance.cpp window_chrome.cpp process_security.cpp)
enh_target_defaults(enh_platform)
target_include_directories(enh_platform PUBLIC include)
target_link_libraries(enh_platform PUBLIC enh_core PRIVATE dwmapi psapi shell32 ole32)
