# Tests unitaires du pipeline (label unit, sans GPU).
add_executable(enh_pipeline_unit_tests
  core_math_layout_test.cpp
  decode_loop_test.cpp
  decoder_selection_test.cpp
  win32_file_test.cpp)
enh_target_defaults(enh_pipeline_unit_tests)
target_include_directories(enh_pipeline_unit_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(enh_pipeline_unit_tests PRIVATE enh_pipeline_core GTest::gtest GTest::gtest_main)
gtest_discover_tests(enh_pipeline_unit_tests PROPERTIES LABELS unit)

# Demux / decodage logiciel : seulement si FFmpeg est present (ADR-037), sinon ignores.
if(TARGET enh_pipeline_ffmpeg)
  add_executable(enh_ffmpeg_unit_tests ffmpeg/clip_writer.cpp ffmpeg/ffmpeg_decode_test.cpp)
  enh_target_defaults(enh_ffmpeg_unit_tests)
  target_include_directories(enh_ffmpeg_unit_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries(enh_ffmpeg_unit_tests PRIVATE enh_pipeline_ffmpeg GTest::gtest GTest::gtest_main)
  enh_copy_runtime_dlls(enh_ffmpeg_unit_tests)
  gtest_discover_tests(enh_ffmpeg_unit_tests PROPERTIES LABELS unit DISCOVERY_TIMEOUT 60)
else()
  message(STATUS "Tests FFmpeg du pipeline ignores : FFmpeg absent.")
endif()
if(TARGET enh_ffmpeg_unit_tests)
  # ffprobe de la MEME build FFmpeg : reference du nombre d'images (critere ENH-008).
  get_property(_enh_ff_bin GLOBAL PROPERTY ENH_FFMPEG_BIN_DIR)
  if(EXISTS "${_enh_ff_bin}/ffprobe.exe")
    target_compile_definitions(enh_ffmpeg_unit_tests PRIVATE ENH_FFPROBE_EXE=L"${_enh_ff_bin}/ffprobe.exe")
  endif()
endif()
