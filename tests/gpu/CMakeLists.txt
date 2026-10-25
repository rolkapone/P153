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
    decode_nvdec_qa_test.cpp
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

# ENH-040 : pont RTX Video (imports sans cudart, VSR 720p -> 4K, debit, VRAM, 10 cycles, modules
# charges, signature, aucun fichier ecrit par NGX). Pont + nvngx_vsr.dll (rel) copies a cote.
add_executable(enh_gpu_rtxvideo_tests
  rtxvideo_test.cpp
  ${PROJECT_SOURCE_DIR}/tests/unit/pipeline/pe/pe_image.cpp)
enh_target_defaults(enh_gpu_rtxvideo_tests)
target_include_directories(enh_gpu_rtxvideo_tests PRIVATE ${PROJECT_SOURCE_DIR}/tests/unit/pipeline)
target_compile_definitions(enh_gpu_rtxvideo_tests PRIVATE ENH_RTXVIDEO_SDK_DIR=L"${ENH_RTXVIDEO_SDK_DIR}")
target_link_libraries(enh_gpu_rtxvideo_tests PRIVATE enh_pipeline_gpu dxgi GTest::gtest GTest::gtest_main)
enh_deploy_rtxvideo(enh_gpu_rtxvideo_tests)
if(TARGET enh_pipeline_ffmpeg)
  enh_copy_runtime_dlls(enh_gpu_rtxvideo_tests)
endif()
gtest_discover_tests(enh_gpu_rtxvideo_tests NO_PRETTY_VALUES PROPERTIES LABELS gpu TIMEOUT 600 DISCOVERY_TIMEOUT 60)

# ENH-047 : enhance-bench lance en processus separe (JSON, codes de sortie, VRAM, sortie VSR).
if(TARGET enhance-bench)
  get_property(_enh_ff_bin GLOBAL PROPERTY ENH_FFMPEG_BIN_DIR)
  add_executable(enh_gpu_bench_tests
    bench_test.cpp
    ${PROJECT_SOURCE_DIR}/tests/unit/pipeline/ffmpeg/clip_writer.cpp)
  enh_target_defaults(enh_gpu_bench_tests)
  target_include_directories(enh_gpu_bench_tests PRIVATE ${PROJECT_SOURCE_DIR}/tests/unit/pipeline)
  target_compile_definitions(enh_gpu_bench_tests PRIVATE
    ENH_BENCH_EXE=L"$<TARGET_FILE:enhance-bench>"
    ENH_CLIPS_DIR=L"${PROJECT_SOURCE_DIR}/tests/clips"
    ENH_FFPROBE_EXE=L"${_enh_ff_bin}/ffprobe.exe"
    ENH_FFMPEG_EXE=L"${_enh_ff_bin}/ffmpeg.exe")
  target_link_libraries(enh_gpu_bench_tests PRIVATE
    enh_bench_lib enh_pipeline_ffmpeg nlohmann_json::nlohmann_json GTest::gtest GTest::gtest_main)
  add_dependencies(enh_gpu_bench_tests enhance-bench)
  enh_copy_runtime_dlls(enh_gpu_bench_tests)
  gtest_discover_tests(enh_gpu_bench_tests NO_PRETTY_VALUES PROPERTIES LABELS gpu TIMEOUT 600 DISCOVERY_TIMEOUT 60)
endif()
