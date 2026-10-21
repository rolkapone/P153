# Options de compilation communes (ADR-022). Utilisation : enh_target_defaults(<cible>)
# Les memes options hote sont passees a nvcc via -Xcompiler pour les fichiers .cu.
function(enh_target_defaults target)
  get_target_property(_type ${target} TYPE)
  if(NOT MSVC OR _type STREQUAL "INTERFACE_LIBRARY")
    return()
  endif()
  set(_cxx /permissive- /Zc:__cplusplus /Zc:preprocessor /utf-8 /W4 /EHsc)
  target_compile_options(${target} PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:${_cxx}>")
  string(REPLACE ";" "," _cuda_host "${_cxx}")
  target_compile_options(${target} PRIVATE "$<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=${_cuda_host}>")
  # Chemins de sources tronques dans __FILE__ (SEC-16).
  target_compile_options(${target} PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:/d1trimfile:${CMAKE_SOURCE_DIR}/>")
  if(ENH_WARNINGS_AS_ERRORS)
    target_compile_options(${target} PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:/WX>")
  endif()
  target_compile_options(${target} PRIVATE
    "$<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CONFIG:Release,RelWithDebInfo>>:/guard:cf;/GS>")
  if(_type STREQUAL "EXECUTABLE" OR _type STREQUAL "SHARED_LIBRARY")
    target_link_options(${target} PRIVATE /DYNAMICBASE /NXCOMPAT /HIGHENTROPYVA /guard:cf)
    # Pas de chemin de build dans le binaire (SEC-16) : le PDB est reference par son seul nom.
    target_link_options(${target} PRIVATE "/PDBALTPATH:%_PDB%")
  endif()
endfunction()
