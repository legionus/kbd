AM_CPPFLAGS = \
	$(CODE_COVERAGE_CPPFLAGS) \
	-I$(srcdir) -I$(builddir) -I$(srcdir)/.. -I$(builddir)/.. \
	-I$(srcdir)/../libcommon -I$(builddir)/../libcommon \
	-I$(srcdir)/../libkbdfile -I$(builddir)/../libkbdfile \
	-DDATADIR=\"$(datadir)\" -DLOCALEDIR=\"$(localedir)\" \
	-U_GNU_SOURCE \
	-funit-at-a-time

AM_CFLAGS = $(CODE_COVERAGE_CFLAGS)

headers = \
	psf.h kfont.h

libkfont_la_SOURCES = \
	$(headers) \
	contextP.h \
	dumpfont.c \
	init.c \
	kdfontop.c \
	kdmapop.c \
	loadunimap.c \
	mapscrn.c \
	psffontop.c \
	setfont.c \
	utf8.c

libkfont_la_LIBADD =  \
	$(builddir)/../libkbdfile/libkbdfile.la \
	$(CODE_COVERAGE_LIBS)

KFONT_CURRENT  = 1
KFONT_REVISION = 0
KFONT_AGE      = 0

if BUILD_LIBKFONT
lib_LTLIBRARIES = libkfont.la
nobase_include_HEADERS = $(headers)
libkfont_la_LDFLAGS = -export-symbols-regex ^kfont_.* -version-info $(KFONT_CURRENT):$(KFONT_REVISION):$(KFONT_AGE)
else
noinst_LTLIBRARIES = libkfont.la
endif
