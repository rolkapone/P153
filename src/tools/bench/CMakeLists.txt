# enhance-bench (ENH-047, ADR-037) : banc de mesure en ligne de commande, outil de developpement
# jamais livre (package.ps1 ne le copie pas).
#  - enh_bench_lib : arguments, BMP, JSON, fichiers, VramMonitor ; sans CUDA, testee en CI ;
#  - enhance-bench : executable console, seulement si ENH_GPU_EFFECTIVE et FFmpeg present.
find_package(nlohmann_json CONFIG QUIET GLOBAL) # GLOBAL : cible visible des tests
if(NOT ENH_HAVE_EXPECTED OR NOT nlohmann_json_FOUND)
  message(STATUS "enhance-bench ignore : tl-expected ou nlohmann-json absent.")
  return()
endif()

add_library(enh_bench_lib STATIC
  lib/args.cpp
  lib/bmp.cpp
  lib/exit_code.cpp
  lib/file_io.cpp
  lib/report.cpp
  lib/vram_monitor.cpp)
enh_target_defaults(enh_bench_lib)
target_include_directories(enh_bench_lib PUBLIC include)
target_link_libraries(enh_bench_lib
  PUBLIC enh_pipeline_core
  PRIVATE nlohmann_json::nlohmann_json psapi)

if(NOT ENH_GPU_EFFECTIVE OR NOT TARGET enh_pipeline_ffmpeg)
  return()
endif()

# SHA du commit au moment du configure (rapport JSON : « git_sha »). Pas de -dirty : le rapport
# QA note l'etat de l'arbre a part.
set(_enh_git_sha "")
find_package(Git QUIET)
if(GIT_FOUND)
  execute_process(COMMAND "${GIT_EXECUTABLE}" rev-parse --short=7 HEAD
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    OUTPUT_VARIABLE _enh_git_sha OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
endif()

add_executable(enhance-bench
  main.cpp
  gpu/decode_stage.cpp
  gpu/gpu_env.cpp
  gpu/vsr_stage.cpp)
enh_target_defaults(enhance-bench)
target_include_directories(enhance-bench PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(enhance-bench PRIVATE
  ENH_BENCH_VERSION="${PROJECT_VERSION}"
  ENH_BENCH_GIT_SHA="${_enh_git_sha}")
target_link_libraries(enhance-bench PRIVATE enh_bench_lib enh_pipeline_gpu dxgi)
# SEC-08 : DLL importees cherchees seulement dans le dossier de l'exe et System32.
target_link_options(enhance-bench PRIVATE /DEPENDENTLOADFLAG:0xA00)
enh_copy_runtime_dlls(enhance-bench) # DLL FFmpeg a cote de l'exe
enh_deploy_rtxvideo(enhance-bench)   # pont + nvngx_vsr.dll (rel) a cote de l'exe (ENH-040)
