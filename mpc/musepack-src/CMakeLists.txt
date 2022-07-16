CMAKE_MINIMUM_REQUIRED(VERSION 2.4)
cmake_policy(SET CMP0003 NEW)
include(TestBigEndian)

project(libmpc C)

set(CMAKE_VERBOSE_MAKEFILE false)
TEST_BIG_ENDIAN(MPC_ENDIANNESS)

if(WIN32)
  option(SHARED "Use shared libmpcdec" OFF)
else(WIN32)
  option(SHARED "Use shared libmpcdec" ON)
endif(WIN32)

add_definitions(-DFAST_MATH -DCVD_FASTLOG)

if(NOT MSVC)
  set(CMAKE_C_FLAGS "-O3 -Wall -fomit-frame-pointer -pipe")
endif(NOT MSVC)

add_subdirectory(libmpcdec)
add_subdirectory(libmpcpsy)
add_subdirectory(libmpcenc)
add_subdirectory(libwavformat)
add_subdirectory(mpcgain)
add_subdirectory(mpcdec)
add_subdirectory(mpcenc)
add_subdirectory(mpc2sv8)
add_subdirectory(mpccut)
add_subdirectory(mpcchap)
add_subdirectory(wavcmp)
add_subdirectory(include)
