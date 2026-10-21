include(GoogleTest)

add_executable(enh_gpu_tests smoke_device_test.cpp)
enh_target_defaults(enh_gpu_tests)
target_link_libraries(enh_gpu_tests PRIVATE CUDA::cudart_static GTest::gtest GTest::gtest_main)
gtest_discover_tests(enh_gpu_tests PROPERTIES LABELS gpu)

# ENH-008 : decodage NVDEC (matrice noyau, vitesse, VRAM, fuites). Clips : tests/clips (non
# commites ; ENH_CLIPS_DIR pour un autre dossier), corpus : tests/gpu/corpus/make-corpus.ps1.
if(TARGET enh_pipeline_ffmpeg)
  get_property(_enh_ff_bin GLOBAL PROPERTY ENH_FFMPEG_BIN_DIR)
  add_executable(enh_gpu_decode_tests
    decode_nvdec_test.cpp
    ${PROJECT_SOURCE_DIR}/tests/unit/pipeline/ffmpeg/clip_writer.cpp)
  enh_target_defaults(enh_gpu_decode_tests)
  target_include_directories(enh_gpu_decode_tests PRIVATE ${PROJECT_SOURCE_DIR}/tests/unit/pipeline)
  target_compile_definitions(enh_gpu_decode_tests PRIVATE
    ENH_CLIPS_DIR=L"${PROJECT_SOURCE_DIR}/tests/clips"
    ENH_FFPROBE_EXE=L"${_enh_ff_bin}/ffprobe.exe")
  target_link_libraries(enh_gpu_decode_tests PRIVATE enh_pipeline_gpu GTest::gtest GTest::gtest_main)
  enh_copy_runtime_dlls(enh_gpu_decode_tests)
  gtest_discover_tests(enh_gpu_decode_tests NO_PRETTY_VALUES PROPERTIES LABELS gpu TIMEOUT 600 DISCOVERY_TIMEOUT 60)
endif()
