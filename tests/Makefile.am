AM_CPPFLAGS = \
	$(CODE_COVERAGE_CPPFLAGS) \
	-I$(srcdir)/../src \
	-I$(srcdir)/../src/libcommon \
	-I$(builddir)/../src/libcommon \
	-I$(srcdir)/../src/libkeymap \
	-I$(builddir)/../src/libkeymap \
	-I$(srcdir)/../src/libkbdfile \
	-I$(builddir)/../src/libkbdfile \
	-DDATADIR=\"$(srcdir)\" -DBUILDDIR=\"$(builddir)\"

AM_CFLAGS = $(CHECK_CFLAGS) $(CODE_COVERAGE_CFLAGS)
LDADD  = $(top_builddir)/src/libkeymap/libkeymap.la $(CHECK_LIBS) @INTLLIBS@ $(CODE_COVERAGE_LIBS)

EXTRA_DIST = \
	alt-is-meta.in \
	dumpkeys-bkeymap.in \
	dumpkeys-fulltable.in \
	dumpkeys-mktable.in \
	\
	alt-is-meta.map \
	charset-keymap0.map \
	dumpkeys.full.ruwin_cplk-UTF-8.map \
	dumpkeys.qwerty-layout.map \
	dumpkeys.ua-ws.map \
	include40.map include41.map \
	keymap0.map keymap1.map keymap2.map keymap3.map keymap4.map keymap5.map keymap6.map \
	VoidSymbol.map \
	\
	alt-is-meta.output \
	dumpkeys.bkeymap.bin \
	dumpkeys.defkeymap.c

#check_SCRIPTS  = dumpkeys-fulltable dumpkeys-mktable dumpkeys-bkeymap alt-is-meta
check_SCRIPTS  = dumpkeys-fulltable alt-is-meta
check_PROGRAMS = libkeymap

noinst_PROGRAMS = \
	libkeymap-dumpkeys libkeymap-mktable libkeymap-bkeymap libkeymap-showmaps \
	$(check_PROGRAMS)

TESTS = $(check_PROGRAMS) $(check_SCRIPTS)
CLEANFILES = $(check_SCRIPTS)

libkeymap_dumpkeys_SOURCES = libkeymap-dumpkeys.c
libkeymap_mktable_SOURCES  = libkeymap-mktable.c
libkeymap_bkeymap_SOURCES  = libkeymap-bkeymap.c
libkeymap_showmaps_SOURCES = libkeymap-showmaps.c

libkeymap_SOURCES = libkeymap.c
libkeymap_LDADD   = $(top_builddir)/src/libkeymap/libkeymap.la $(LDADD)

%: %.in
	sed \
		-e 's,@VERSION@,$(VERSION),g' \
		-e 's,@DATADIR@,$(srcdir),g' \
		-e 's,@BUILDDIR@,$(builddir),g' \
		<$< >$(builddir)/$@
	touch -r $< $(builddir)/$@
	chmod 755 $(builddir)/$@
