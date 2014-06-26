AM_CPPFLAGS = \
	-I$(srcdir) -I$(builddir) -I$(srcdir)/.. -I$(builddir)/.. \
	-DDATADIR=\"$(datadir)\" -DLOCALEDIR=\"$(localedir)\" \
	-U_GNU_SOURCE -D_FORTIFY_SOURCE=2 \
	-funit-at-a-time
CLEANFILES = libkeymap.pc
EXTRA_DIST = libkeymap.pc.in

headers = \
	keymap.h \
	keymap/array.h \
	keymap/context.h \
	keymap/charset.h \
	keymap/common.h \
	keymap/dump.h \
	keymap/kernel.h \
	keymap/kmap.h \
	keymap/logging.h \
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
	array.c \
	findfile.c common.c kernel.c dump.c kmap.c diacr.c func.c summary.c loadkeys.c \
	contextP.h \
	parser.y parser.h analyze.l analyze.h \
	modifiers.c modifiers.h \
	ksyms.c ksyms.h $(ksyms_headers) \
	$(srcdir)/../nls.h $(srcdir)/../paths.h $(srcdir)/../kbd.h

if BUILD_LIBKEYMAP
pkgconfigdir   = $(libdir)/pkgconfig
pkgconfig_DATA = libkeymap.pc

lib_LTLIBRARIES = libkeymap.la
nobase_include_HEADERS = $(headers)
libkeymap_la_LDFLAGS = -export-symbols-regex ^lk_.*
else
noinst_LTLIBRARIES = libkeymap.la
endif

AM_LFLAGS = --header-file=$(CURDIR)/analyze.h
BUILT_SOURCES = parser.c analyze.c parser.h analyze.h
CLEANFILES += $(BUILT_SOURCES)

parser.h: parser.c ; @true
analyze.h: analyze.c ; @true

%.pc: %.pc.in Makefile
	sed \
	    -e 's,[@]VERSION[@],$(VERSION),g' \
	    -e 's,[@]prefix[@],$(prefix),g' \
	    -e 's,[@]exec_prefix[@],$(exec_prefix),g' \
	    -e 's,[@]libdir[@],$(libdir),g' \
	    -e 's,[@]includedir[@],$(includedir),g' \
	    <$< >$(builddir)/$@
	touch -r $< $(builddir)/$@
	chmod 644 $(builddir)/$@
