AM_CPPFLAGS = \
	$(CODE_COVERAGE_CPPFLAGS) \
	-I$(srcdir)/../src \
	-I$(srcdir)/../src/libcommon \
	-I$(builddir)/../src/libcommon \
	-I$(srcdir)/../src/libkeymap \
	-I$(builddir)/../src/libkeymap \
	-I$(srcdir)/../src/libkbdfile \
	-I$(builddir)/../src/libkbdfile \
	-DDATADIR=\"$(srcdir)\" \
	-DABS_DATADIR=\"$(realpath $(srcdir))\" \
	-DBUILDDIR=\"$(builddir)\"

AM_CFLAGS = $(CHECK_CFLAGS) $(CODE_COVERAGE_CFLAGS)
LDADD  = \
	$(top_builddir)/src/libcommon/libcommon.a \
	$(top_builddir)/src/libkeymap/libkeymap.la \
	$(top_builddir)/src/libkbdfile/libkbdfile.la \
	@INTLLIBS@ $(CODE_COVERAGE_LIBS)

EXTRA_DIST = \
	alt-is-meta.in \
	dumpkeys-bkeymap.in \
	dumpkeys-fulltable.in \
	dumpkeys-mktable.in \
	\
	data/alt-is-meta/alt-is-meta.map \
	data/alt-is-meta/alt-is-meta.output \
	data/dumpkeys-bkeymap/bkeymap.bin \
	data/dumpkeys-fulltable/qwerty-layout.map \
	data/dumpkeys-fulltable/ruwin_cplk-UTF-8.map \
	data/dumpkeys-fulltable/ua-ws.map \
	data/dumpkeys-mktable/defkeymap.c \
	data/libkeymap/charset-keymap0.map \
	data/libkeymap/include40.map \
	data/libkeymap/include41.map \
	data/libkeymap/keymap0.map \
	data/libkeymap/keymap1.map \
	data/libkeymap/keymap2.map \
	data/libkeymap/keymap3.map \
	data/libkeymap/keymap4.map \
	data/libkeymap/keymap5.map \
	data/libkeymap/keymap6.map \
	data/VoidSymbol.map \
	findfile

NULL =

check_SCRIPTS  = \
	alt-is-meta        \
	dumpkeys-bkeymap   \
	dumpkeys-fulltable \
	dumpkeys-mktable   \
	$(NULL)

check_PROGRAMS = \
	libkbdfile-test01 \
	libkbdfile-test02 \
	libkbdfile-test03 \
	libkbdfile-test04 \
	libkbdfile-test05 \
	libkbdfile-test06 \
	libkbdfile-test07 \
	libkbdfile-test08 \
	libkbdfile-test09 \
	libkbdfile-test10 \
	libkbdfile-test11 \
	libkbdfile-test12 \
	libkeymap-test01  \
	libkeymap-test02  \
	libkeymap-test03  \
	libkeymap-test04  \
	libkeymap-test05  \
	libkeymap-test06  \
	libkeymap-test07  \
	libkeymap-test08  \
	libkeymap-test09  \
	libkeymap-test10  \
	libkeymap-test11  \
	libkeymap-test12  \
	libkeymap-test13  \
	libkeymap-test14  \
	libkeymap-test15  \
	libkeymap-test16  \
	libkeymap-test17  \
	libkeymap-test18  \
	$(NULL)

noinst_PROGRAMS = $(check_PROGRAMS) \
	libkeymap-dumpkeys libkeymap-mktable libkeymap-bkeymap libkeymap-showmaps \
	$(NULL)

TESTS = $(check_PROGRAMS) $(check_SCRIPTS)
CLEANFILES = $(check_SCRIPTS)

libkeymap_dumpkeys_SOURCES = libkeymap-dumpkeys.c
libkeymap_mktable_SOURCES  = libkeymap-mktable.c
libkeymap_bkeymap_SOURCES  = libkeymap-bkeymap.c
libkeymap_showmaps_SOURCES = libkeymap-showmaps.c

libkbdfile_test01_SOURCES = libkbdfile-test01.c
libkbdfile_test02_SOURCES = libkbdfile-test02.c
libkbdfile_test03_SOURCES = libkbdfile-test03.c
libkbdfile_test04_SOURCES = libkbdfile-test04.c
libkbdfile_test05_SOURCES = libkbdfile-test05.c
libkbdfile_test06_SOURCES = libkbdfile-test06.c
libkbdfile_test07_SOURCES = libkbdfile-test07.c
libkbdfile_test08_SOURCES = libkbdfile-test08.c
libkbdfile_test09_SOURCES = libkbdfile-test09.c
libkbdfile_test10_SOURCES = libkbdfile-test10.c
libkbdfile_test11_SOURCES = libkbdfile-test11.c
libkbdfile_test12_SOURCES = libkbdfile-test12.c

libkeymap_test01_SOURCES = libkeymap-test01.c
libkeymap_test02_SOURCES = libkeymap-test02.c
libkeymap_test03_SOURCES = libkeymap-test03.c
libkeymap_test04_SOURCES = libkeymap-test04.c
libkeymap_test05_SOURCES = libkeymap-test05.c
libkeymap_test06_SOURCES = libkeymap-test06.c
libkeymap_test07_SOURCES = libkeymap-test07.c
libkeymap_test08_SOURCES = libkeymap-test08.c
libkeymap_test09_SOURCES = libkeymap-test09.c
libkeymap_test10_SOURCES = libkeymap-test10.c
libkeymap_test11_SOURCES = libkeymap-test11.c
libkeymap_test12_SOURCES = libkeymap-test12.c
libkeymap_test13_SOURCES = libkeymap-test13.c
libkeymap_test14_SOURCES = libkeymap-test14.c
libkeymap_test15_SOURCES = libkeymap-test15.c
libkeymap_test16_SOURCES = libkeymap-test16.c
libkeymap_test17_SOURCES = libkeymap-test17.c
libkeymap_test18_SOURCES = libkeymap-test18.c

%: %.in
	sed \
		-e 's,@VERSION@,$(VERSION),g' \
		-e 's,@DATADIR@,$(srcdir),g' \
		-e 's,@BUILDDIR@,$(builddir),g' \
		<$< >$(builddir)/$@
	touch -r $< $(builddir)/$@
	chmod 755 $(builddir)/$@
