EXTRA_DIST = CMakeLists.txt

AM_CPPFLAGS = -I$(top_srcdir)/include

if HAVE_VISIBILITY
  AM_CFLAGS = -fvisibility=hidden
endif

METASOURCES = AUTO
noinst_LIBRARIES = libmpcpsy.a
libmpcpsy_a_SOURCES = ans.c cvd.c fft4g.c fft_routines.c psy.c psy_tab.c \
	profile.c libmpcpsy.h
