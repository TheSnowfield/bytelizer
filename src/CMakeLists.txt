project(bytelizer)

# add source files as static library
add_library(${PROJECT_NAME}_static STATIC
  ${BYTELIZER_SRC}
)

# add source files as shared library
add_library(${PROJECT_NAME} SHARED
  ${BYTELIZER_SRC}
)
