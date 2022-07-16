include_directories(${libmpc_SOURCE_DIR}/include)

include_directories(${libmpc_SOURCE_DIR}/libwavformat)
link_directories(${libmpc_BINARY_DIR}/libwavformat)
include_directories(${libmpc_SOURCE_DIR}/libmpcdec)
link_directories(${libmpc_BINARY_DIR}/libmpcdec)

if(MSVC)
include_directories(${libmpc_SOURCE_DIR}/win32)
add_executable(mpcdec_cmd mpcdec.c ${libmpc_SOURCE_DIR}/win32/attgetopt)
endif(MSVC)

add_executable(mpcdec_cmd mpcdec.c)

if(NOT MSVC)
target_link_libraries(mpcdec_cmd m)
endif(NOT MSVC)

target_link_libraries(mpcdec_cmd wavformat_static)

if(SHARED)
  target_link_libraries(mpcdec_cmd mpcdec)
else(SHARED)
  target_link_libraries(mpcdec_cmd mpcdec_static)
endif(SHARED)

SET_TARGET_PROPERTIES(mpcdec_cmd PROPERTIES OUTPUT_NAME "mpcdec")

install(TARGETS mpcdec_cmd RUNTIME DESTINATION bin)
