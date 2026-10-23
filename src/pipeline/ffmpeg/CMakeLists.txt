# enh_pipeline_ffmpeg (ADR-037) : sans CUDA, testable sans GPU.
add_library(enh_pipeline_ffmpeg STATIC
  av_error.cpp
  av_mapping.cpp
  avio_reader.cpp
  ffmpeg_decoder.cpp
  ffmpeg_demuxer.cpp
  frame_store.cpp)
enh_target_defaults(enh_pipeline_ffmpeg)
target_include_directories(enh_pipeline_ffmpeg PUBLIC include)
target_link_libraries(enh_pipeline_ffmpeg PUBLIC enh_pipeline_core Enhance::ffmpeg)
