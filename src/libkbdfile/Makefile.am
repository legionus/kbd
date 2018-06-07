AM_CPPFLAGS = \
	$(CODE_COVERAGE_CPPFLAGS) \
	-I$(srcdir) -I$(builddir) -I$(srcdir)/.. -I$(builddir)/.. \
	-DDATADIR=\"$(datadir)\" -DLOCALEDIR=\"$(localedir)\" \
	-U_GNU_SOURCE \
	-funit-at-a-time

AM_CFLAGS = $(CODE_COVERAGE_CFLAGS)
LDADD     = $(CODE_COVERAGE_LIBS)

headers = \
	keymap/findfile.h

libkbdfile_la_SOURCES = \
	$(headers) $(srcdir)/../nls.h \
	findfile.c

if BUILD_LIBKEYMAP
lib_LTLIBRARIES = libkbdfile.la
nobase_include_HEADERS = $(headers)
libkbdfile_la_LDFLAGS = -export-symbols-regex ^lk_.*
else
noinst_LTLIBRARIES = libkbdfile.la
endif
