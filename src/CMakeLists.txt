project(bytelizer)

# add include directories
include_directories(
  ${CMAKE_SOURCE_DIR}
  ${BYTELIZER_LIBRARY_DIR}
  ${BYTELIZER_ADDITIONAL_INC_DIR}
)

file(GLOB_RECURSE BYTELIZER_SRC src/*.c)
  list(APPEND BYTELIZER_SRC ${BYTELIZER_LIB_SRC})

# add source files as static library
add_library(${PROJECT_NAME}_static STATIC
  ${BYTELIZER_SRC}
)

# add source files as shared library
add_library(${PROJECT_NAME} SHARED
  ${BYTELIZER_SRC}
)
