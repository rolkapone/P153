# FFmpeg LGPL partage (ADR-025, ADR-037). Cibles importees FFmpeg::<lib> (DLL + .lib d'import) et
# Enhance::ffmpeg (ensemble). Racine : -DENH_FFMPEG_ROOT= > variable d'environnement
# ENH_FFMPEG_ROOT > third_party/ffmpeg/lgpl-shared (build BtbN de developpement). Absent : les
# cibles n'existent pas, enh_pipeline_ffmpeg et ses tests ne sont pas construits (CI sans FFmpeg).

set(_ENH_FFMPEG_THIS_DIR "${CMAKE_CURRENT_LIST_DIR}")

function(enh_find_ffmpeg)
  if(TARGET Enhance::ffmpeg)
    return()
  endif()
  if(ENH_FFMPEG_ROOT)
    set(_root "${ENH_FFMPEG_ROOT}")
  elseif(DEFINED ENV{ENH_FFMPEG_ROOT} AND NOT "$ENV{ENH_FFMPEG_ROOT}" STREQUAL "")
    set(_root "$ENV{ENH_FFMPEG_ROOT}")
  else()
    get_filename_component(_root "${_ENH_FFMPEG_THIS_DIR}/../third_party/ffmpeg/lgpl-shared" ABSOLUTE)
  endif()
  if(NOT EXISTS "${_root}/include/libavcodec/avcodec.h")
    message(STATUS "FFmpeg absent (${_root}) : enh_pipeline_ffmpeg non construite (ADR-037).")
    return()
  endif()
  # swresample : dependance d execution d avcodec/avformat (DLL copiee a cote des executables).
  set(_libs avutil swresample avcodec avformat swscale)
  foreach(_lib IN LISTS _libs)
    file(GLOB _dll "${_root}/bin/${_lib}-*.dll")
    if(NOT _dll OR NOT EXISTS "${_root}/lib/${_lib}.lib")
      message(STATUS "FFmpeg incomplet (${_lib} : DLL ou .lib manquante) : enh_pipeline_ffmpeg non construite.")
      return()
    endif()
    list(GET _dll 0 _dll0)
    add_library(FFmpeg::${_lib} SHARED IMPORTED GLOBAL)
    set_target_properties(FFmpeg::${_lib} PROPERTIES
      IMPORTED_LOCATION "${_dll0}"
      IMPORTED_IMPLIB "${_root}/lib/${_lib}.lib"
      INTERFACE_INCLUDE_DIRECTORIES "${_root}/include")
  endforeach()
  add_library(Enhance::ffmpeg INTERFACE IMPORTED GLOBAL)
  target_link_libraries(Enhance::ffmpeg INTERFACE FFmpeg::avformat FFmpeg::avcodec FFmpeg::swscale FFmpeg::swresample FFmpeg::avutil)
  set_property(GLOBAL PROPERTY ENH_FFMPEG_BIN_DIR "${_root}/bin")
  message(STATUS "FFmpeg : ${_root}")
endfunction()

# Copie les DLL importees (FFmpeg...) a cote de l'executable, AVANT la decouverte des tests
# GoogleTest (appeler avant gtest_discover_tests).
function(enh_copy_runtime_dlls target)
  add_custom_command(TARGET ${target} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:${target}> $<TARGET_FILE_DIR:${target}>
    COMMAND_EXPAND_LISTS)
endfunction()
