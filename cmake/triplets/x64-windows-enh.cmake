# Triplet maison (ADR-023/024) : bibliotheques statiques, CRT dynamique (/MD), release seulement.
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_BUILD_TYPE release)

# Qt et FFmpeg restent en DLL (LGPL : liaison dynamique obligatoire).
if(PORT MATCHES "^qt" OR PORT STREQUAL "ffmpeg")
  set(VCPKG_LIBRARY_LINKAGE dynamic)
endif()
