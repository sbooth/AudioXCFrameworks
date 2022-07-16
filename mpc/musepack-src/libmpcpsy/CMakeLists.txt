include_directories(${libmpc_SOURCE_DIR}/include)
add_library(mpcpsy_static STATIC ans cvd fft4g fft_routines profile psy psy_tab ${libmpc_SOURCE_DIR}/common/fastmath)
