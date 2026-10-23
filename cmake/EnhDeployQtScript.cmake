# Script (cmake -P) appele par enh_deploy_qt_runtime() : voir EnhDeployQt.cmake.
cmake_minimum_required(VERSION 3.30)
if(POLICY CMP0207)
  cmake_policy(SET CMP0207 NEW) # chemins normalises (/) avant les filtres de GET_RUNTIME_DEPENDENCIES
endif()

foreach(_v EXE QT_BIN QT_PLUGINS QT_QML SCANNER QML_ROOT DUMPBIN)
  if(NOT DEFINED ${_v})
    message(FATAL_ERROR "EnhDeployQtScript : ${_v} manquant")
  endif()
endforeach()

get_filename_component(_dest "${EXE}" DIRECTORY)
set(_dlls_to_scan "${EXE}")

function(_enh_copy src dst_dir)
  file(MAKE_DIRECTORY "${dst_dir}")
  get_filename_component(_name "${src}" NAME)
  file(COPY_FILE "${src}" "${dst_dir}/${_name}" ONLY_IF_DIFFERENT)
endfunction()

# 1. Plugins Qt strictement necessaires (liste fermee).
foreach(_plugin platforms/qwindows.dll iconengines/qsvgicon.dll imageformats/qsvg.dll)
  if(EXISTS "${QT_PLUGINS}/${_plugin}")
    get_filename_component(_sub "${_plugin}" DIRECTORY)
    _enh_copy("${QT_PLUGINS}/${_plugin}" "${_dest}/plugins/${_sub}")
    list(APPEND _dlls_to_scan "${QT_PLUGINS}/${_plugin}")
  else()
    message(FATAL_ERROR "Plugin Qt introuvable : ${_plugin}")
  endif()
endforeach()

# 2. Modules QML importes par l'application (dependances comprises).
execute_process(
  COMMAND "${SCANNER}" -rootPath "${QML_ROOT}" -importPath "${QT_QML}"
  OUTPUT_VARIABLE _json
  RESULT_VARIABLE _rc
  ERROR_QUIET)
if(NOT _rc EQUAL 0)
  message(FATAL_ERROR "qmlimportscanner a echoue (${_rc})")
endif()
string(JSON _count LENGTH "${_json}")
math(EXPR _last "${_count} - 1")
set(_module_dirs)
foreach(_i RANGE ${_last})
  string(JSON _type ERROR_VARIABLE _err GET "${_json}" ${_i} type)
  string(JSON _path ERROR_VARIABLE _err2 GET "${_json}" ${_i} path)
  if(NOT _type STREQUAL "module" OR _err2 OR NOT _path)
    continue()
  endif()
  file(TO_CMAKE_PATH "${_path}" _path)
  string(FIND "${_path}" "${QT_QML}/" _pos)
  if(NOT _pos EQUAL 0)
    continue() # modules de l'application : compiles dans l'exe
  endif()
  list(APPEND _module_dirs "${_path}")
endforeach()
list(REMOVE_DUPLICATES _module_dirs)

foreach(_dir IN LISTS _module_dirs)
  file(RELATIVE_PATH _rel "${QT_QML}" "${_dir}")
  # Fichiers du module seulement (les sous-modules sont listes a part) ; ni .pdb ni .qmltypes.
  file(GLOB _files LIST_DIRECTORIES false "${_dir}/*")
  foreach(_f IN LISTS _files)
    if(_f MATCHES "\\.(pdb|qmltypes)$")
      continue()
    endif()
    _enh_copy("${_f}" "${_dest}/qml/${_rel}")
    if(_f MATCHES "\\.dll$")
      list(APPEND _dlls_to_scan "${_f}")
    endif()
  endforeach()
endforeach()

# 3. DLL dont dependent l'exe, les plugins et les modules QML, prises dans le dossier bin de Qt.
set(CMAKE_GET_RUNTIME_DEPENDENCIES_PLATFORM "windows+pe")
set(CMAKE_GET_RUNTIME_DEPENDENCIES_TOOL "dumpbin")
set(CMAKE_GET_RUNTIME_DEPENDENCIES_COMMAND "${DUMPBIN}")
list(GET _dlls_to_scan 0 _exe)
list(REMOVE_AT _dlls_to_scan 0)
file(GET_RUNTIME_DEPENDENCIES
  EXECUTABLES "${_exe}"
  LIBRARIES ${_dlls_to_scan}
  DIRECTORIES "${QT_BIN}"
  RESOLVED_DEPENDENCIES_VAR _resolved
  UNRESOLVED_DEPENDENCIES_VAR _unresolved
  PRE_EXCLUDE_REGEXES "^api-ms-" "^ext-ms-"
  POST_INCLUDE_REGEXES "^${QT_BIN}/"
  POST_EXCLUDE_REGEXES ".*")
foreach(_dll IN LISTS _resolved)
  _enh_copy("${_dll}" "${_dest}")
endforeach()

# 4. Controle C2 : jamais de module reseau dans le dossier de sortie.
file(GLOB_RECURSE _network LIST_DIRECTORIES false "${_dest}/Qt6Network*.dll")
if(_network)
  message(FATAL_ERROR "Qt6Network present dans le dossier de sortie (interdit, ADR-024) : ${_network}")
endif()
