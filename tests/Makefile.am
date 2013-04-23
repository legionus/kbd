AM_CPPFLAGS = \
	-I$(srcdir)/../src/libkeymap \
	-I$(builddir)/../src/libkeymap \
	-DDATADIR=\"$(srcdir)\" -DBUILDDIR=\"$(builddir)\"

AM_CFLAGS = $(CHECK_CFLAGS)
LDADD  = $(top_builddir)/src/libkeymap/libkeymap.la $(CHECK_LIBS)

EXTRA_DIST = \
	dumpkeys-fulltable.in \
	\
	charset-keymap0.map \
	dumpkeys.full.ruwin_cplk-UTF-8.map \
	dumpkeys.qwerty-layout.map \
	dumpkeys.ua-ws.map \
	include40.map include41.map \
	keymap0.map keymap1.map keymap2.map keymap3.map keymap4.map keymap5.map keymap6.map \
	VoidSymbol.map

noinst_PROGRAMS = libkeymap-dumpkeys

check_SCRIPTS  = dumpkeys-fulltable
check_PROGRAMS = libkeymap-init libkeymap-kmap libkeymap-keys libkeymap-parse \
	libkeymap-charset

TESTS = $(check_PROGRAMS) $(check_SCRIPTS)
CLEANFILES = $(check_SCRIPTS)

libkeymap_init_SOURCES     = libkeymap-init.c
libkeymap_kmap_SOURCES     = libkeymap-kmap.c
libkeymap_keys_SOURCES     = libkeymap-keys.c
libkeymap_parse_SOURCES    = libkeymap-parse.c
libkeymap_charset_SOURCES  = libkeymap-charset.c
libkeymap_dumpkeys_SOURCES = libkeymap-dumpkeys.c

%: %.in
	sed \
		-e 's,@VERSION@,$(VERSION),g' \
		-e 's,@DATADIR@,$(srcdir),g' \
		-e 's,@BUILDDIR@,$(builddir),g' \
		<$< >$(builddir)/$@
	touch -r $< $(builddir)/$@
	chmod --reference=$< $(builddir)/$@
