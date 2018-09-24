AM_CPPFLAGS = \
	$(CODE_COVERAGE_CPPFLAGS) \
	-I$(srcdir) -I$(builddir) -I$(srcdir)/.. -I$(builddir)/.. \
	-I$(srcdir)/../libcommon -I$(builddir)/../libcommon \
	-I$(srcdir)/../libkbdfile -I$(builddir)/../libkbdfile \
	-DDATADIR=\"$(datadir)\" -DLOCALEDIR=\"$(localedir)\" \
	-U_GNU_SOURCE \
	-funit-at-a-time

AM_CFLAGS = $(CODE_COVERAGE_CFLAGS)
LDADD = \
	$(top_builddir)/src/libcommon/libcommon.a \
	$(CODE_COVERAGE_LIBS)

libkfont_la_SOURCES = \
	psf.h \
	init.c kfont.h contextP.h \
	psffontop.c psffontop.h \
	kdfontop.c \
	kdmapop.c \
	loadunimap.c loadunimap.h \
	mapscrn.c mapscrn.h \
	utf8.c utf8.h

noinst_LTLIBRARIES = libkfont.la
