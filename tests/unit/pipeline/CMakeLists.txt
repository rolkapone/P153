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

# ENH-040 : pont RTX Video sans GPU (faux pont, fausses DLL pont, controle statique SEC-01).
if(TARGET enh_pipeline_rtxvideo)
  # Fausses DLL pont : chargement reel (LoadLibraryExW + GetProcAddress) et controle d'ABI.
  foreach(_variant ok bad_abi partial)
    set(_def rtxvideo/fake_bridge.def)
    if(_variant STREQUAL "partial")
      set(_def rtxvideo/fake_bridge_partial.def)
    endif()
    add_library(enh_fake_bridge_${_variant} SHARED rtxvideo/fake_bridge_dll.cpp ${_def})
    target_link_libraries(enh_fake_bridge_${_variant} PRIVATE enh_rtxvideo_bridge_api)
  endforeach()
  target_compile_definitions(enh_fake_bridge_bad_abi PRIVATE ENH_FAKE_ABI=99)
  target_compile_definitions(enh_fake_bridge_partial PRIVATE ENH_FAKE_PARTIAL)

  add_executable(enh_rtxvideo_unit_tests
    fakes/fake_bridge.cpp
    pe/pe_image.cpp
    rtxvideo/ngx_static_policy_test.cpp
    rtxvideo/rtxvideo_unit_test.cpp)
  enh_target_defaults(enh_rtxvideo_unit_tests)
  target_include_directories(enh_rtxvideo_unit_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  target_compile_definitions(enh_rtxvideo_unit_tests PRIVATE
    ENH_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
    ENH_FAKE_BRIDGE_OK=L"enh_fake_bridge_ok.dll"
    ENH_FAKE_BRIDGE_BAD_ABI=L"enh_fake_bridge_bad_abi.dll"
    ENH_FAKE_BRIDGE_PARTIAL=L"enh_fake_bridge_partial.dll")
  target_link_libraries(enh_rtxvideo_unit_tests PRIVATE enh_pipeline_rtxvideo GTest::gtest GTest::gtest_main)
  add_dependencies(enh_rtxvideo_unit_tests enh_fake_bridge_ok enh_fake_bridge_bad_abi enh_fake_bridge_partial)
  gtest_discover_tests(enh_rtxvideo_unit_tests PROPERTIES LABELS unit)
endif()
