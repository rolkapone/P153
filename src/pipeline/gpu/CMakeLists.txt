# Pipeline GPU reel : n'existe que si ENH_GPU_EFFECTIVE (SDK NVIDIA + CUDA presents).
add_library(enh_pipeline_gpu STATIC gpu_factory.cpp cuda/probe.cu)
enh_target_defaults(enh_pipeline_gpu)
target_link_libraries(enh_pipeline_gpu PUBLIC
  enh_pipeline_core
  CUDA::cudart_static
  NVIDIA::rtx_video NVIDIA::video_codec NVIDIA::optical_flow NVIDIA::tensorrt)
set_target_properties(enh_pipeline_gpu PROPERTIES CUDA_SEPARABLE_COMPILATION OFF)

# Decodage NVDEC (ENH-008) : hwaccel FFmpeg sur notre contexte CUDA ; exige enh_pipeline_ffmpeg.
if(TARGET enh_pipeline_ffmpeg)
  target_sources(enh_pipeline_gpu PRIVATE
    nvdec/cuda_frame_store.cpp
    nvdec/cuda_raii.cpp
    nvdec/nvdec_decoder.cpp)
  target_include_directories(enh_pipeline_gpu PUBLIC nvdec/include)
  target_link_libraries(enh_pipeline_gpu PUBLIC enh_pipeline_ffmpeg)
endif()
