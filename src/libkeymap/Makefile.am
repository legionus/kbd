AM_CPPFLAGS = \
	-I$(srcdir) -I$(builddir) -I$(srcdir)/.. -I$(builddir)/.. \
	-DDATADIR=\"$(datadir)\" -DLOCALEDIR=\"$(localedir)\" \
	-funit-at-a-time

headers = \
	keymap.h \
	keymap/data.h \
	keymap/base.h \
	keymap/dump.h \
	keymap/load.h \
	keymap/logging.h \
	keymap/parse.h \
	keymap/findfile.h

ksyms_headers = \
	syms.ktyp.h \
	syms.synonyms.h \
	syms.cp1250.h \
	syms.ethiopic.h \
	syms.iso8859_15.h \
	syms.iso8859_5.h \
	syms.iso8859_7.h \
	syms.iso8859_8.h \
	syms.iso8859_9.h \
	syms.koi8.h \
	syms.latin1.h \
	syms.latin2.h \
	syms.latin3.h \
	syms.latin4.h \
	syms.mazovia.h \
	syms.sami.h \
	syms.thai.h

libkeymap_la_SOURCES = \
	$(headers) \
	findfile.c \
	base.c load.c dump.c \
	parser.y parser.h analyze.l analyze.h parseP.h \
	modifiers.c modifiers.h \
	ksyms.c ksyms.h $(ksyms_headers) \
	summary.c \
	$(srcdir)/../nls.h $(srcdir)/../paths.h $(srcdir)/../kbd.h

if BUILD_LIBKEYMAP
lib_LTLIBRARIES = libkeymap.la
nobase_include_HEADERS = $(headers)
#libkeymap_la_LDFLAGS = -export-symbols-regex ^lk_.*
else
noinst_LTLIBRARIES = libkeymap.la
endif

AM_LFLAGS = --header-file=$(CURDIR)/analyze.h
BUILT_SOURCES = parser.c analyze.c parser.h analyze.h
CLEANFILES = $(BUILT_SOURCES)
parser.h: parser.c ; @true
analyze.h: analyze.c ; @true
