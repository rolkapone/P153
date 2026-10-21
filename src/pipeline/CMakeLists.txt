# Pipeline video : api (en-tetes) / core + fake (testes en CI) / ffmpeg (si FFmpeg present, sans
# CUDA, ADR-037) / gpu (seulement si ENH_WITH_GPU).
add_library(enh_pipeline_api INTERFACE)
target_include_directories(enh_pipeline_api INTERFACE api/include)
target_link_libraries(enh_pipeline_api INTERFACE enh_common)

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
target_link_libraries(enh_pipeline_core PUBLIC enh_pipeline_api)

add_library(enh_pipeline_fake STATIC fake/fake_pipeline.cpp)
enh_target_defaults(enh_pipeline_fake)
target_link_libraries(enh_pipeline_fake PUBLIC enh_pipeline_core)

# enh_pipeline_ffmpeg : demux, AVIO maison, decodage (logiciel ; NVDEC branche par enh_pipeline_gpu).
include(EnhFfmpeg)
enh_find_ffmpeg()
if(TARGET Enhance::ffmpeg AND ENH_HAVE_EXPECTED)
  add_subdirectory(ffmpeg)
endif()

# enh_pipeline_impl : le pipeline concret, reel ou simule (ADR-036).
add_library(enh_pipeline_impl INTERFACE)
if(ENH_GPU_EFFECTIVE)
  add_subdirectory(gpu)
  target_link_libraries(enh_pipeline_impl INTERFACE enh_pipeline_gpu)
else()
  target_link_libraries(enh_pipeline_impl INTERFACE enh_pipeline_fake)
endif()
