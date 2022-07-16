FIND_PATH(REPLAY_GAIN_INCLUDE_DIR replaygain/gain_analysis.h
	/usr/include
	/usr/local/include
	${libmpc_SOURCE_DIR}/../libreplaygain/include
)

FIND_LIBRARY(REPLAY_GAIN_LIBRARY NAMES replaygain replaygain_static PATHS
	/usr/lib
	/usr/local/lib
	${libmpc_SOURCE_DIR}/../libreplaygain/bin/src/release
)

include_directories(${libmpc_SOURCE_DIR}/include)
include_directories(${REPLAY_GAIN_INCLUDE_DIR})

link_directories(${libmpc_BINARY_DIR}/libmpcdec)

add_executable(mpcgain mpcgain.c)

target_link_libraries(mpcgain ${REPLAY_GAIN_LIBRARY})

if(SHARED)
  target_link_libraries(mpcgain mpcdec)
else(SHARED)
  target_link_libraries(mpcgain mpcdec_static)
endif(SHARED)

if(NOT MSVC)
target_link_libraries(mpcgain m)
endif(NOT MSVC)

install(TARGETS mpcgain RUNTIME DESTINATION bin)
