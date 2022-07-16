if(MPC_ENDIANNESS)
add_definitions(-DMPC_BIG_ENDIAN)
endif(MPC_ENDIANNESS)

include_directories(${libmpc_SOURCE_DIR}/include)
include_directories(${libmpc_SOURCE_DIR}/libmpcpsy)
include_directories(${libmpc_SOURCE_DIR}/libmpcenc)

link_directories(${libmpc_BINARY_DIR}/libmpcpsy)
link_directories(${libmpc_BINARY_DIR}/libmpcenc)

add_executable(mpcenc keyboard.c mpcenc.c pipeopen.c stderr.c ${libmpc_SOURCE_DIR}/common/tags.c wave_in.c winmsg.c)

target_link_libraries(mpcenc mpcpsy_static)
target_link_libraries(mpcenc mpcenc_static)

if(WIN32)
target_link_libraries(mpcenc winmm)
endif(WIN32)

if(NOT MSVC)
target_link_libraries(mpcenc m)
endif(NOT MSVC)

if(CMAKE_SYSTEM_NAME STREQUAL OpenBSD)
target_link_libraries(mpcenc ossaudio)
endif(CMAKE_SYSTEM_NAME STREQUAL OpenBSD)

install(TARGETS mpcenc RUNTIME DESTINATION bin)
