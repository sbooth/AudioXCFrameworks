include_directories(${libmpc_SOURCE_DIR}/include)

include_directories(${libmpc_SOURCE_DIR}/libmpcdec)
link_directories(${libmpc_BINARY_DIR}/libmpcdec)
include_directories(${libmpc_SOURCE_DIR}/libmpcenc)
link_directories(${libmpc_BINARY_DIR}/libmpcenc)

if(MSVC)
include_directories(${libmpc_SOURCE_DIR}/win32)
add_executable(mpc2sv8 mpc2sv8.c ${libmpc_SOURCE_DIR}/win32/attgetopt.c ${libmpc_SOURCE_DIR}/win32/basename.c ${libmpc_SOURCE_DIR}/win32/dirent.c)
endif(MSVC)

add_executable(mpc2sv8 mpc2sv8.c)

if(SHARED)
  target_link_libraries(mpc2sv8 mpcdec)
else(SHARED)
  target_link_libraries(mpc2sv8 mpcdec_static)
endif(SHARED)

target_link_libraries(mpc2sv8 mpcenc_static)

if(WIN32)
target_link_libraries(mpc2sv8 winmm)
endif(WIN32)

if(NOT MSVC)
target_link_libraries(mpc2sv8 m)
endif(NOT MSVC)

if(CMAKE_SYSTEM_NAME STREQUAL OpenBSD)
target_link_libraries(mpc2sv8 ossaudio)
endif(CMAKE_SYSTEM_NAME STREQUAL OpenBSD)

install(TARGETS mpc2sv8 RUNTIME DESTINATION bin)
