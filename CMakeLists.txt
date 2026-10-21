# Enhance - racine CMake (ADR-020, 021, 022, 023, 026, 036)
cmake_minimum_required(VERSION 3.30)

# vcpkg (ADR-023) : active seulement si VCPKG_ROOT existe. Doit preceder project().
if(NOT CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{VCPKG_ROOT}
   AND EXISTS "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE FILEPATH "")
endif()

# AUTO | ON | OFF (ADR-020). CUDA n'est active que si le GPU est effectif.
set(ENH_WITH_GPU "AUTO" CACHE STRING "Pipeline GPU reel : AUTO, ON ou OFF")
set_property(CACHE ENH_WITH_GPU PROPERTY STRINGS AUTO ON OFF)
set(ENH_WITH_UI "AUTO" CACHE STRING "Interface Qt Quick : AUTO, ON ou OFF")
set_property(CACHE ENH_WITH_UI PROPERTY STRINGS AUTO ON OFF)
option(ENH_BUILD_TESTS "Compiler les tests" ON)
option(ENH_WARNINGS_AS_ERRORS "Avertissements = erreurs (/WX)" OFF)
set(ENH_NVIDIA_SDK_DIR "" CACHE PATH "Racine des SDK NVIDIA (defaut : third_party/nvidia)")
set(ENH_FFMPEG_ROOT "" CACHE PATH "Racine de FFmpeg LGPL partage (defaut : third_party/ffmpeg/lgpl-shared)")

# Variables passees par les presets, utilisees seulement avec vcpkg (evite l avertissement CMake).
set(_enh_vcpkg_vars "${VCPKG_TARGET_TRIPLET}${VCPKG_HOST_TRIPLET}${VCPKG_OVERLAY_TRIPLETS}${VCPKG_MANIFEST_FEATURES}")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(EnhNvidia)

# Decision GPU effective, avant project() pour savoir si CUDA est demande.
set(ENH_GPU_EFFECTIVE OFF)
if(NOT ENH_WITH_GPU STREQUAL "OFF")
  enh_check_nvidia_sdks(_enh_missing)
  if(_enh_missing)
    if(ENH_WITH_GPU STREQUAL "ON")
      message(FATAL_ERROR "ENH_WITH_GPU=ON mais des SDK NVIDIA manquent :\n${_enh_missing}")
    else()
      message(WARNING "GPU desactive (ENH_WITH_GPU=AUTO) :\n${_enh_missing}")
    endif()
  else()
    set(ENH_GPU_EFFECTIVE ON)
  endif()
endif()

if(ENH_GPU_EFFECTIVE)
  project(Enhance VERSION 0.1.0 LANGUAGES CXX CUDA)
  # sm_120 (RTX 50) en binaire + PTX pour compatibilite avant (ADR-021).
  set(CMAKE_CUDA_ARCHITECTURES "120-real;120-virtual")
  set(CMAKE_CUDA_STANDARD 20)
  set(CMAKE_CUDA_RUNTIME_LIBRARY Static)
  find_package(CUDAToolkit REQUIRED)
  enh_define_nvidia_targets()
else()
  project(Enhance VERSION 0.1.0 LANGUAGES CXX)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL") # /MD partout (ADR-022)

include(EnhWarnings)

# Dependances vcpkg (vcpkg.json). Absentes en phase A : on degrade proprement.
find_package(tl-expected CONFIG QUIET)
if(tl-expected_FOUND)
  set(ENH_HAVE_EXPECTED ON)
else()
  set(ENH_HAVE_EXPECTED OFF)
  message(STATUS "tl-expected absent (vcpkg non installe) : le code qui depend de enh::Expected n'est pas compile.")
endif()

find_package(GTest CONFIG QUIET)
if(GTest_FOUND)
  set(ENH_HAVE_GTEST ON)
else()
  set(ENH_HAVE_GTEST OFF)
  message(STATUS "GoogleTest absent (vcpkg non installe) : tests unitaires non compiles.")
endif()

# Interface Qt : AUTO = si Qt6 est trouve.
set(ENH_UI_EFFECTIVE OFF)
if(NOT ENH_WITH_UI STREQUAL "OFF")
  find_package(Qt6 QUIET COMPONENTS Quick)
  if(Qt6_FOUND)
    set(ENH_UI_EFFECTIVE ON)
  elseif(ENH_WITH_UI STREQUAL "ON")
    message(FATAL_ERROR "ENH_WITH_UI=ON mais Qt6 est introuvable. Lancer scripts/bootstrap.ps1 (vcpkg, feature ui).")
  else()
    message(STATUS "Qt6 absent : interface non compilee (ENH_WITH_UI=AUTO).")
  endif()
endif()

include(CTest)
add_subdirectory(src)
if(ENH_BUILD_TESTS)
  add_subdirectory(tests)
endif()

enh_print_summary()
