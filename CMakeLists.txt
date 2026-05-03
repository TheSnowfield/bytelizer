cmake_minimum_required(VERSION 3.10)

project(bytelizer)
set(CMAKE_C_STANDARD 17)

# all useful directories
set(BYTELIZER_ADDITIONAL_INC_DIR "")
set(BYTELIZER_LIBRARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(BYTELIZER_BITCOMPILER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bitc)

list(APPEND BYTELIZER_ADDITIONAL_INC_DIR ${BYTELIZER_THIRDLIB_DIR})
list(APPEND BYTELIZER_ADDITIONAL_INC_DIR ${BYTELIZER_PREBUILD_DIR})

# build config
if(NOT DEFINED CONFIG OR CONFIG STREQUAL "debug")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")
elseif()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
endif()

# check platform endianness
include(TestBigEndian)
TEST_BIG_ENDIAN(BIG_ENDIAN)
if(BIG_ENDIAN)
  add_definitions(-DBYTELIZER_ENDIANNESS=1)
elseif()
  add_definitions(-DBYTELIZER_ENDIANNESS=0)
endif()

if(BUILD STREQUAL "lib")
  include(${BYTELIZER_LIBRARY_DIR}/CMakeLists.txt)
elseif(BUILD STREQUAL "bitc")
  include(${BYTELIZER_BITCOMPILER_DIR}/CMakeLists.txt)
elseif(BUILD STREQUAL "all")
	include(${BYTELIZER_LIBRARY_DIR}/CMakeLists.txt)
	include(${BYTELIZER_BITCOMPILER_DIR}/CMakeLists.txt)
else()
  message(FATAL_ERROR "Unknown build type, please specify `-DBUILD=lib|bitc|all`")
endif()
