# Detection des SDK NVIDIA (ADR-026). Les SDK ne sont jamais commites : third_party/nvidia/.
# Priorite : -DENH_NVIDIA_SDK_DIR= > variable d'environnement ENH_NVIDIA_SDK_DIR > third_party/nvidia.
#
# Mode verification (utilise par scripts/test-gpu.ps1) :
#   cmake -DENH_NVIDIA_SDK_DIR=<dir> -P cmake/EnhNvidia.cmake

# nom | fichier sentinelle (relatif au dossier du SDK) | dossier d'inclusion
set(_ENH_NVIDIA_SDKS
  "rtx-video-sdk|include/nvsdk_ngx.h|include"
  "video-codec-sdk|Interface/nvEncodeAPI.h|Interface"
  "optical-flow-sdk|NvOFInterface/nvOpticalFlowCommon.h|NvOFInterface"
  "tensorrt|include/NvInfer.h|include")

set(_ENH_NVIDIA_THIS_DIR "${CMAKE_CURRENT_LIST_DIR}")

function(enh_nvidia_root out)
  if(ENH_NVIDIA_SDK_DIR)
    set(_root "${ENH_NVIDIA_SDK_DIR}")
  elseif(DEFINED ENV{ENH_NVIDIA_SDK_DIR} AND NOT "$ENV{ENH_NVIDIA_SDK_DIR}" STREQUAL "")
    set(_root "$ENV{ENH_NVIDIA_SDK_DIR}")
  else()
    get_filename_component(_root "${_ENH_NVIDIA_THIS_DIR}/../third_party/nvidia" ABSOLUTE)
  endif()
  set(${out} "${_root}" PARENT_SCOPE)
endfunction()

# Message d'erreur fixe (ADR-026).
function(enh_nvidia_missing_message out name root sentinel)
  set(${out}
    "SDK NVIDIA manquant : ${name}. Attendu : ${root}/${name}/${sentinel}. Voir docs/gpu/spike.md section 2.1. Pour compiler sans GPU : -DENH_WITH_GPU=OFF"
    PARENT_SCOPE)
endfunction()

# Renvoie dans <out_missing> les messages d'erreur (vide si tout est present).
function(enh_check_nvidia_sdks out_missing)
  enh_nvidia_root(_root)
  set(_missing "")
  foreach(_entry IN LISTS _ENH_NVIDIA_SDKS)
    string(REPLACE "|" ";" _parts "${_entry}")
    list(GET _parts 0 _name)
    list(GET _parts 1 _sentinel)
    if(NOT EXISTS "${_root}/${_name}/${_sentinel}")
      enh_nvidia_missing_message(_msg "${_name}" "${_root}" "${_sentinel}")
      string(APPEND _missing "  - ${_msg}\n")
    endif()
  endforeach()
  set(${out_missing} "${_missing}" PARENT_SCOPE)
endfunction()

# Cree NVIDIA::<alias> (INTERFACE IMPORTED) : rtx_video, video_codec, optical_flow, tensorrt.
function(enh_define_nvidia_targets)
  enh_nvidia_root(_root)
  foreach(_entry IN LISTS _ENH_NVIDIA_SDKS)
    string(REPLACE "|" ";" _parts "${_entry}")
    list(GET _parts 0 _name)
    list(GET _parts 2 _inc)
    string(REPLACE "-sdk" "" _alias "${_name}")
    string(REPLACE "-" "_" _alias "${_alias}")
    if(NOT TARGET NVIDIA::${_alias})
      add_library(NVIDIA::${_alias} INTERFACE IMPORTED GLOBAL)
      # SYSTEM : les avertissements des en-tetes NVIDIA ne cassent pas /WX
      target_include_directories(NVIDIA::${_alias} SYSTEM INTERFACE "${_root}/${_name}/${_inc}")
    endif()
  endforeach()
  # Seules bibliotheques d'import necessaires ; le reste est charge dynamiquement (ADR-026).
  if(EXISTS "${_root}/rtx-video-sdk/lib/Windows/x64/nvsdk_ngx_d.lib")
    target_link_libraries(NVIDIA::rtx_video INTERFACE
      "${_root}/rtx-video-sdk/lib/Windows/x64/nvsdk_ngx_d.lib")
  endif()
  file(GLOB _nvinfer "${_root}/tensorrt/lib/nvinfer_[0-9]*.lib")
  if(_nvinfer)
    list(GET _nvinfer 0 _nvinfer0)
    target_link_libraries(NVIDIA::tensorrt INTERFACE "${_nvinfer0}")
  endif()
endfunction()

function(enh_print_summary)
  message(STATUS "---- Enhance ----")
  message(STATUS "  GPU effectif : ${ENH_GPU_EFFECTIVE} (ENH_WITH_GPU=${ENH_WITH_GPU})")
  message(STATUS "  UI effective : ${ENH_UI_EFFECTIVE} (ENH_WITH_UI=${ENH_WITH_UI})")
  message(STATUS "  Tests        : ${ENH_BUILD_TESTS} (GoogleTest : ${ENH_HAVE_GTEST})")
  if(ENH_GPU_EFFECTIVE)
    enh_nvidia_root(_root)
    message(STATUS "  SDK NVIDIA   : ${_root}")
    message(STATUS "  CUDA         : ${CUDAToolkit_VERSION}")
    file(STRINGS "${_root}/tensorrt/include/NvInferVersion.h" _trt REGEX "define TRT_(MAJOR|MINOR|PATCH)_ENTERPRISE")
    message(STATUS "  TensorRT     : ${_trt}")
  endif()
endfunction()

# Mode script : verification seule ; echec (code 1) si un SDK manque.
if(CMAKE_SCRIPT_MODE_FILE AND CMAKE_SCRIPT_MODE_FILE STREQUAL CMAKE_CURRENT_LIST_FILE)
  enh_nvidia_root(_root)
  set(_bad FALSE)
  foreach(_entry IN LISTS _ENH_NVIDIA_SDKS)
    string(REPLACE "|" ";" _parts "${_entry}")
    list(GET _parts 0 _name)
    list(GET _parts 1 _sentinel)
    if(EXISTS "${_root}/${_name}/${_sentinel}")
      message(STATUS "SDK ${_name} : OK")
    else()
      message(STATUS "SDK ${_name} : MANQUANT (attendu ${_root}/${_name}/${_sentinel})")
      set(_bad TRUE)
    endif()
  endforeach()
  if(_bad)
    message(FATAL_ERROR "SDK NVIDIA manquants. Voir docs/gpu/spike.md section 2.1.")
  endif()
endif()
