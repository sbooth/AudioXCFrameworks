EXTRA_DIST = CMakeLists.txt

AM_CPPFLAGS = -I$(top_srcdir)/include

if HAVE_VISIBILITY
  AM_CFLAGS = -fvisibility=hidden
endif

METASOURCES = AUTO
noinst_LIBRARIES = libmpcenc.a
libmpcenc_a_SOURCES = analy_filter.c bitstream.c encode_sv7.c huffsv7.c quant.c libmpcenc.h
