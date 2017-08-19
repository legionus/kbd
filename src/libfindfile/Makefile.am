WARNINGS = -Wall -Wextra -W -Wshadow -Wcast-align \
	-Wwrite-strings -Wconversion -Waggregate-return -Wstrict-prototypes \
	-Wmissing-prototypes -Wmissing-declarations -Wmissing-noreturn \
	-Wmissing-format-attribute -Wredundant-decls -Wdisabled-optimization

AM_CPPFLAGS = \
	$(WARNINGS) \
	-I$(srcdir) -I$(builddir) -I$(srcdir)/.. -I$(builddir)/.. \
	-DDATADIR=\"$(datadir)\" -DLOCALEDIR=\"$(localedir)\" \
	-U_GNU_SOURCE -D_FORTIFY_SOURCE=2 \
	-funit-at-a-time

headers = \
	keymap/findfile.h

libfindfile_la_SOURCES = \
	$(headers) $(srcdir)/../nls.h \
	findfile.c

if BUILD_LIBKEYMAP
lib_LTLIBRARIES = libfindfile.la
nobase_include_HEADERS = $(headers)
libfindfile_la_LDFLAGS = -export-symbols-regex ^lk_.*
else
noinst_LTLIBRARIES = libfindfile.la
endif
