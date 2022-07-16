EXTRA_DIST = CMakeLists.txt

if HAVE_VISIBILITY
  AM_CFLAGS = -fvisibility=hidden
endif

METASOURCES = AUTO
noinst_LIBRARIES = libwavformat.a

libwavformat_a_SOURCES = input.c output.c libwaveformat.h
