SET(COMPILE_FLAGS "-fPIC")
SET(LINK_FLAGS    "-lPIC")

SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${COMPILE_FLAGS}" )
SET( CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${LINK_FLAGS}" )

add_llvm_loadable_module( visualize
  visualize.cpp
  visualize_helpers.cpp

  DEPENDS
  intrinsics_gen
)
