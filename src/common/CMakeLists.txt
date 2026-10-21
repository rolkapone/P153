# enh_common : en-tetes partages par enh_core et le pipeline (ADR-037). Aucune dependance vers le haut.
add_library(enh_common INTERFACE)
add_library(enh::common ALIAS enh_common)
target_include_directories(enh_common INTERFACE include)
if(ENH_HAVE_EXPECTED)
  target_link_libraries(enh_common INTERFACE tl::expected)
  target_compile_definitions(enh_common INTERFACE ENH_HAVE_EXPECTED=1)
endif()
