# Pipeline video : api (en-tetes) / core + fake (testes en CI) / ffmpeg (si FFmpeg present, sans
# CUDA, ADR-037) / gpu (seulement si ENH_WITH_GPU).
add_library(enh_pipeline_api INTERFACE)
target_include_directories(enh_pipeline_api INTERFACE api/include)
target_link_libraries(enh_pipeline_api INTERFACE enh_common)

# Interfaces d'etage INTERNES au pipeline (IDemuxer, IDecoder, VideoFrame...) : cible separee, que
# l'application (enh_core, enh_app) ne lie pas (amendement ADR-037, revue ENH-008).
add_library(enh_pipeline_stage INTERFACE)
target_include_directories(enh_pipeline_stage INTERFACE stage/include)
target_link_libraries(enh_pipeline_stage INTERFACE enh_pipeline_api)

set(_enh_core_sources core/pipeline_core.cpp)
if(ENH_HAVE_EXPECTED)
  list(APPEND _enh_core_sources
    core/decode_loop.cpp
    core/decoder_selection.cpp
    core/frame_layout.cpp
    core/host_frame_pool.cpp
    core/log.cpp
    core/win32_file.cpp)
endif()
add_library(enh_pipeline_core STATIC ${_enh_core_sources})
enh_target_defaults(enh_pipeline_core)
target_include_directories(enh_pipeline_core PUBLIC core/include)
target_link_libraries(enh_pipeline_core PUBLIC enh_pipeline_stage)

add_library(enh_pipeline_fake STATIC fake/fake_pipeline.cpp)
enh_target_defaults(enh_pipeline_fake)
target_link_libraries(enh_pipeline_fake PUBLIC enh_pipeline_core)

# API C du pont RTX Video (en-tete C pur, sans CUDA ni NGX) : partagee par la DLL pont et par
# enh_pipeline_rtxvideo (ADR-037, ENH-040).
add_library(enh_rtxvideo_bridge_api INTERFACE)
target_include_directories(enh_rtxvideo_bridge_api INTERFACE rtxvideo_bridge/include)

# enh_pipeline_rtxvideo : chargement du pont, RAII NGX, RtxVideoUpscaler, controle des modules et
# des signatures. Aucune dependance CUDA : construite et testee en CI avec un faux pont.
if(ENH_HAVE_EXPECTED)
  add_library(enh_pipeline_rtxvideo STATIC
    rtxvideo/bridge_api.cpp
    rtxvideo/bridge_library.cpp
    rtxvideo/module_trust.cpp
    rtxvideo/ngx_guard.cpp
    rtxvideo/ngx_runtime.cpp
    rtxvideo/rtxvideo_upscaler.cpp)
  enh_target_defaults(enh_pipeline_rtxvideo)
  target_include_directories(enh_pipeline_rtxvideo PUBLIC rtxvideo/include)
  target_link_libraries(enh_pipeline_rtxvideo
    PUBLIC enh_pipeline_core enh_rtxvideo_bridge_api
    PRIVATE wintrust crypt32 shell32 ole32)
endif()

# enh_pipeline_ffmpeg : demux, AVIO maison, decodage (logiciel ; NVDEC branche par enh_pipeline_gpu).
include(EnhFfmpeg)
enh_find_ffmpeg()
if(TARGET Enhance::ffmpeg AND ENH_HAVE_EXPECTED)
  add_subdirectory(ffmpeg)
endif()

# enh_pipeline_impl : le pipeline concret, reel ou simule (ADR-036).
add_library(enh_pipeline_impl INTERFACE)
if(ENH_GPU_EFFECTIVE)
  add_subdirectory(rtxvideo_bridge)
  add_subdirectory(gpu)
  target_link_libraries(enh_pipeline_impl INTERFACE enh_pipeline_gpu)
else()
  target_link_libraries(enh_pipeline_impl INTERFACE enh_pipeline_fake)
endif()
