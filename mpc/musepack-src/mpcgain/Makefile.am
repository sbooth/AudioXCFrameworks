EXTRA_DIST = CMakeLists.txt

AM_CPPFLAGS = -I$(top_srcdir)/include

AM_CFLAGS = -ffast-math

if HAVE_VISIBILITY
  AM_CFLAGS += -fvisibility=hidden
endif


METASOURCES = AUTO
bin_PROGRAMS = mpcgain
mpcgain_LDADD = -lm -lreplaygain \
	$(top_builddir)/libmpcdec/libmpcdec.la
mpcgain_SOURCES = mpcgain.c
