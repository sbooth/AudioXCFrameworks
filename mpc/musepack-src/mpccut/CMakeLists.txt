include_directories(${libmpc_SOURCE_DIR}/include)

include_directories(${libmpc_SOURCE_DIR}/libmpcdec)
link_directories(${libmpc_BINARY_DIR}/libmpcdec)
include_directories(${libmpc_SOURCE_DIR}/libmpcenc)
link_directories(${libmpc_BINARY_DIR}/libmpcenc)

if(MSVC)
include_directories(${libmpc_SOURCE_DIR}/win32)
add_executable(mpccut mpccut ${libmpc_SOURCE_DIR}/win32/attgetopt)
endif(MSVC)

add_executable(mpccut mpccut.c)

if(SHARED)
  target_link_libraries(mpccut mpcdec)
else(SHARED)
  target_link_libraries(mpccut mpcdec_static)
endif(SHARED)

target_link_libraries(mpccut mpcenc_static)

if(NOT MSVC)
target_link_libraries(mpccut m)
endif(NOT MSVC)

install(TARGETS mpccut RUNTIME DESTINATION bin)
