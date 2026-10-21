# Pipeline video : api (en-tetes) / core + fake (testes en CI) / gpu (seulement si ENH_WITH_GPU).
add_library(enh_pipeline_api INTERFACE)
target_include_directories(enh_pipeline_api INTERFACE api/include)

add_library(enh_pipeline_core STATIC core/pipeline_core.cpp)
enh_target_defaults(enh_pipeline_core)
target_link_libraries(enh_pipeline_core PUBLIC enh_pipeline_api)

add_library(enh_pipeline_fake STATIC fake/fake_pipeline.cpp)
enh_target_defaults(enh_pipeline_fake)
target_link_libraries(enh_pipeline_fake PUBLIC enh_pipeline_core)

# enh_pipeline_impl : le pipeline concret, reel ou simule (ADR-036).
add_library(enh_pipeline_impl INTERFACE)
if(ENH_GPU_EFFECTIVE)
  add_subdirectory(gpu)
  target_link_libraries(enh_pipeline_impl INTERFACE enh_pipeline_gpu)
else()
  target_link_libraries(enh_pipeline_impl INTERFACE enh_pipeline_fake)
endif()
