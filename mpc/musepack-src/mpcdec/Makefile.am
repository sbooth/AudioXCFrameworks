EXTRA_DIST = CMakeLists.txt

AM_CPPFLAGS = -I$(top_srcdir)/libwavformat -I$(top_srcdir)/include

if HAVE_VISIBILITY
  AM_CFLAGS = -fvisibility=hidden
endif

METASOURCES = AUTO
bin_PROGRAMS = mpcdec
mpcdec_SOURCES = mpcdec.c
mpcdec_LDADD = -lm \
	$(top_builddir)/libmpcdec/libmpcdec.la \
	$(top_builddir)/libwavformat/libwavformat.a
