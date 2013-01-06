AM_CPPFLAGS = -DDATADIR=\"$(datadir)\" -DLOCALEDIR=\"$(localedir)\" \
	-funit-at-a-time

ALL_S   = ../nls.h ../paths.h ../kbd.h

if BUILD_LIBKEYMAP
lib_LTLIBRARIES = libkeymap.la
else
noinst_LTLIBRARIES = libkeymap.la
endif

libkeymap_la_SOURCES = \
	findfile.c findfile.h \
	parse.y analyze.l \
	load.c \
	dump.c keymap.h \
	modifiers.c modifiers.h \
	ksyms.c ksyms.h cp1250.syms.h ethiopic.syms.h koi8.syms.h sami.syms.h thai.syms.h \
	summary.c \
	$(ALL_S)

AM_LFLAGS = --header-file=$(CURDIR)/analyze.h
BUILT_SOURCES = parse.c analyze.c parse.h analyze.h
CLEANFILES = $(BUILT_SOURCES)

