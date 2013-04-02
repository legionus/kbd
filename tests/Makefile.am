AM_CPPFLAGS = \
	-I$(srcdir)/../src/libkeymap \
	-I$(builddir)/../src/libkeymap

CFLAGS = $(CHECK_CFLAGS)
LDADD  = $(top_builddir)/src/libkeymap/libkeymap.la $(CHECK_LIBS)

check_PROGRAMS = libkeymap-init libkeymap-kmap libkeymap-keys libkeymap-parse
TESTS = $(check_PROGRAMS)

libkeymap_init_SOURCES  = libkeymap-init.c
libkeymap_kmap_SOURCES  = libkeymap-kmap.c
libkeymap_keys_SOURCES  = libkeymap-keys.c
libkeymap_parse_SOURCES = libkeymap-parse.c
