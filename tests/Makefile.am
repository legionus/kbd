AM_CPPFLAGS = \
	-I$(srcdir)/../src/libkeymap \
	-I$(builddir)/../src/libkeymap

AM_CFLAGS = $(CHECK_CFLAGS)
LDADD  = $(top_builddir)/src/libkeymap/libkeymap.la $(CHECK_LIBS)

noinst_PROGRAMS = libkeymap-dumpkeys

check_PROGRAMS = libkeymap-init libkeymap-kmap libkeymap-keys libkeymap-parse
TESTS = $(check_PROGRAMS) \
	dumpkeys-fulltable

libkeymap_init_SOURCES  = libkeymap-init.c
libkeymap_kmap_SOURCES  = libkeymap-kmap.c
libkeymap_keys_SOURCES  = libkeymap-keys.c
libkeymap_parse_SOURCES = libkeymap-parse.c

libkeymap_dumpkeys_SOURCES = libkeymap-dumpkeys.c
