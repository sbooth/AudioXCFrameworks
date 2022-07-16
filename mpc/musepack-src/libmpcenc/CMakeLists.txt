include_directories(${libmpc_SOURCE_DIR}/include)
add_library(mpcenc_static STATIC analy_filter bitstream encode_sv7 huffsv7 quant ${libmpc_SOURCE_DIR}/common/crc32)
