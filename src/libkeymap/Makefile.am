AM_CPPFLAGS = \
	-I$(srcdir) -I$(builddir) -I$(srcdir)/.. -I$(builddir)/.. \
	-DDATADIR=\"$(datadir)\" -DLOCALEDIR=\"$(localedir)\" \
	-funit-at-a-time

if BUILD_LIBKEYMAP
lib_LTLIBRARIES = libkeymap.la
else
noinst_LTLIBRARIES = libkeymap.la
endif

libkeymap_la_SOURCES = \
	findfile.c findfile.h \
	parse.y analyze.l parse.h analyze.h \
	load.c \
	dump.c keymap.h  keymapP.h \
	modifiers.c modifiers.h \
	ksyms.c ksyms.h syms.cp1250.h syms.ethiopic.h syms.koi8.h syms.sami.h syms.thai.h \
	summary.c \
	$(srcdir)/../nls.h $(srcdir)/../paths.h $(srcdir)/../kbd.h

AM_LFLAGS = --header-file=$(CURDIR)/analyze.h
BUILT_SOURCES = parse.c analyze.c parse.h analyze.h
CLEANFILES = $(BUILT_SOURCES)
parse.h: parse.c ; @true
analyze.h: analyze.c ; @true
