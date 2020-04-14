AM_CPPFLAGS = \
	$(CODE_COVERAGE_CPPFLAGS) \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/libcommon \
	-I$(top_srcdir)/src/libkbdfile \
	-DDATADIR=\"$(datadir)\" \
	-funit-at-a-time

AM_CFLAGS = $(CODE_COVERAGE_CFLAGS)

LDADD = \
	$(top_builddir)/src/libcommon/libcommon.a \
	$(CODE_COVERAGE_LIBS)

libkfont_la_SOURCES = \
	kfont.h \
	psf.h psffontop.c psffontop.h \
	context.c context.h \
	unicode.c unicode.h \
	utf8.c utf8.h \
	kdmapop.c \
	loadunimap.c \
	mapscrn.c \
	setfont.c \
	kdfontop.c

noinst_LTLIBRARIES = libkfont.la
