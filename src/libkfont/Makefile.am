# SPDX-License-Identifier: LGPL-2.0-or-later
#
# Copyright (C) 2020  Alexey Gladkov <gladkov.alexey@gmail.com>
#
include $(top_srcdir)/Makefile.common

CLEANFILES = libkfont.pc
EXTRA_DIST = libkfont.pc.gen libkfont.map

AM_CPPFLAGS += -I$(srcdir)

KFONT_CURRENT  = 2
KFONT_REVISION = 0
KFONT_AGE      = 0

KFONT_SHAREDLIB = libkfont.so
KFONT_SONAME    = $(KFONT_SHAREDLIB).$(KFONT_CURRENT)
KFONT_MAPFILE   = $(top_srcdir)/src/libkfont/libkfont.map

headers = \
	../include/kbd/compiler_attributes.h \
	../include/kfont.h \
	../include/kbd/kfont.h

libkfont_la_SOURCES = \
	$(headers) \
	kfontP.h \
	psffontop.c \
	psfxtable.c \
	context.c \
	unicode.c \
	utf8.c utf8.h \
	kdmapop.c \
	loadunimap.c \
	mapscrn.c \
	setfont.c \
	kdfontop.c

libkfont_la_LIBADD = $(builddir)/../libkbdfile/libkbdfile.la
libkfont_la_LDFLAGS = -Wl,-soname,$(KFONT_SONAME),--version-script=$(KFONT_MAPFILE),-z,defs,-stats

if BUILD_LIBKFONT
pkgconfigdir   = $(libdir)/pkgconfig
pkgconfig_DATA = libkfont.pc

nobase_include_HEADERS = $(headers)
lib_LTLIBRARIES = libkfont.la
else
noinst_LTLIBRARIES = libkfont.la
endif
