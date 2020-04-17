# SPDX-License-Identifier: LGPL-2.0-or-later
#
# Copyright (C) 2020  Alexey Gladkov <gladkov.alexey@gmail.com>
#
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

KFONT_CURRENT  = 1
KFONT_REVISION = 0
KFONT_AGE      = 0

KFONT_SHAREDLIB = libkfont.so
KFONT_SONAME    = $(KFONT_SHAREDLIB).$(KFONT_CURRENT)
KFONT_MAPFILE   = $(top_srcdir)/src/libkfont/libkfont.map

libkfont_la_SOURCES = \
	kfont.h kfontP.h \
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

libkfont_la_LDFLAGS = -Wl,-soname,$(KFONT_SONAME),--version-script=$(KFONT_MAPFILE),-z,defs,-stats

noinst_LTLIBRARIES = libkfont.la
