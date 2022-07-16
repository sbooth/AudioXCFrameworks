AM_CPPFLAGS = -I$(top_srcdir)/libwavformat

if HAVE_VISIBILITY
  AM_CFLAGS = -fvisibility=hidden
endif

METASOURCES = AUTO
bin_PROGRAMS = wavcmp
wavcmp_SOURCES = wavcmp.c
wavcmp_LDADD = $(top_builddir)/libwavformat/libwavformat.a
