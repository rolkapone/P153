# tests/unit : sans GPU (CI). tests/gpu : RTX requise, local seulement (label gpu).
if(ENH_HAVE_GTEST)
  add_subdirectory(unit)
  if(ENH_GPU_EFFECTIVE)
    add_subdirectory(gpu)
  endif()
else()
  message(STATUS "Tests ignores : GoogleTest introuvable (installer vcpkg via scripts/bootstrap.ps1).")
endif()
