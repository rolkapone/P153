# enh_rtxvideo_bridge.dll (ADR-037, docs/gpu/ENH-040.md) : isole NGX (RTX Video SDK, compile avec
# CUDA 11.8) du runtime CUDA 13 de l'application. DLL a part, CRT statique (/MT, impose par
# nvsdk_ngx_s.lib), API pilote CUDA seulement : JAMAIS cudart. Chargee explicitement par
# enh_pipeline_rtxvideo (LoadLibraryExW, chemin absolu), jamais liee a la compilation.
enh_nvidia_root(_enh_nv_root)
set(_enh_rtx "${_enh_nv_root}/rtx-video-sdk")

add_library(enh_rtxvideo_bridge SHARED bridge.cpp cudart_shim.cpp enh_rtxvideo_bridge.def)
enh_target_defaults(enh_rtxvideo_bridge)
set_target_properties(enh_rtxvideo_bridge PROPERTIES
  MSVC_RUNTIME_LIBRARY "MultiThreaded"   # derogation locale au /MD de l'ADR-020 (ADR-037)
  CXX_VISIBILITY_PRESET hidden)
target_compile_definitions(enh_rtxvideo_bridge PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
target_include_directories(enh_rtxvideo_bridge PRIVATE include)
target_include_directories(enh_rtxvideo_bridge SYSTEM PRIVATE "${_enh_rtx}/include")
# cuda.lib (nvcuda.dll) ; nvsdk_ngx_s.lib importe aussi advapi32 / user32 / shell32.
target_link_libraries(enh_rtxvideo_bridge PRIVATE
  CUDA::cuda_driver
  "${_enh_rtx}/lib/Windows/x64/nvsdk_ngx_s.lib"
  advapi32 user32 shell32)
# SEC-08 : les imports statiques du pont (nvcuda.dll...) ne sont resolus que dans System32.
target_link_options(enh_rtxvideo_bridge PRIVATE /DEPENDENTLOADFLAG:0x800)
# Garde-fou : aucun cudart ne doit entrer dans le pont (cause du bloquant B4).
get_target_property(_enh_bridge_libs enh_rtxvideo_bridge LINK_LIBRARIES)
if(_enh_bridge_libs MATCHES "cudart")
  message(FATAL_ERROR "enh_rtxvideo_bridge ne doit pas lier cudart (ENH-040, bloquant B4)")
endif()

# Deploiement a cote d'un executable : le pont + nvngx_vsr.dll du dossier `rel` UNIQUEMENT (la DLL
# `dev` affiche un filigrane et n'est jamais livree, SEC-08). TrueHDR viendra avec ENH-032.
set(ENH_RTXVIDEO_FEATURE_DLL "${_enh_rtx}/bin/Windows/x64/rel/nvngx_vsr.dll" CACHE INTERNAL "")
set(ENH_RTXVIDEO_SDK_DIR "${_enh_rtx}" CACHE INTERNAL "")
function(enh_deploy_rtxvideo target)
  add_dependencies(${target} enh_rtxvideo_bridge)
  add_custom_command(TARGET ${target} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:enh_rtxvideo_bridge> "${ENH_RTXVIDEO_FEATURE_DLL}" $<TARGET_FILE_DIR:${target}>
    VERBATIM)
endfunction()
