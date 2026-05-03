project(bitc)

# search source files
file(GLOB_RECURSE BYTELIZER_BITCOMPILER_SRC ${BYTELIZER_BITCOMPILER_DIR}/*.c)
  list(APPEND BYTELIZER_BITCOMPILER_SRC ${BYTELIZER_LIB_SRC})

# add include directories
include_directories(
  ${BYTELIZER_BITCOMPILER_DIR}
)

# executable
add_executable(${PROJECT_NAME} ${BYTELIZER_BITCOMPILER_SRC})
target_include_directories(bitc PRIVATE include)
