cmake_minimum_required(VERSION 3.16)
project(bitc LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4 /O2)
else()
    add_compile_options(-Wall -Wextra -O2)
endif()

# ── sources ──
set(BITC_SOURCES
    src/bitc.c
    src/lexer.c
    src/ast.c
    src/compiler.c
    src/generator.c
    src/utils.c
    src/codegen/mapping.c
    src/codegen/types.c
    src/codegen/plugin.c
    src/pass/validate.c
    src/pass/fold.c
    src/pass/merge.c
)

# ── executable ──
add_executable(bitc ${BITC_SOURCES})
target_include_directories(bitc PRIVATE include)
