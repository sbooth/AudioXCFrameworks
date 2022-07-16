EXTRA_DIST = CMakeLists.txt

bin_PROGRAMS = mpcchap

common_sources = ../common/tags.c ../common/crc32.c

AM_CPPFLAGS = -I$(top_srcdir)/include

if HAVE_VISIBILITY
  AM_CFLAGS = -fvisibility=hidden
endif

METASOURCES = AUTO
mpcchap_SOURCES = dictionary.c iniparser.c mpcchap.c $(common_sources) \
dictionary.h iniparser.h

mpcchap_LDADD = $(top_builddir)/libmpcdec/libmpcdec.la \
	$(top_builddir)/libmpcenc/libmpcenc.a \
	-lm -lcuefile
