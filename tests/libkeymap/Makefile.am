NULL =

AM_CPPFLAGS = \
	-D_GNU_SOURCE \
	$(CODE_COVERAGE_CPPFLAGS) \
	-I$(top_srcdir)/src/include \
	-I$(top_srcdir)/src/libcommon \
	-I$(top_srcdir)/src/libkeymap \
	-DTESTDIR=\"$(realpath $(top_srcdir))/tests\"

AM_CFLAGS = $(CHECK_CFLAGS) $(CODE_COVERAGE_CFLAGS)
AM_LDFLAGS = $(CODE_COVERAGE_CFLAGS)

LDADD  = \
	$(top_builddir)/src/libcommon/libcommon.a \
	$(top_builddir)/src/libkbdfile/libkbdfile.la \
	$(top_builddir)/src/libkeymap/libkeymap.la \
	@LIBINTL@ $(CODE_COVERAGE_LIBS)

noinst_HEADERS = libkeymap-test.h

noinst_PROGRAMS = \
	libkeymap-test01 \
	libkeymap-test02 \
	libkeymap-test03 \
	libkeymap-test04 \
	libkeymap-test05 \
	libkeymap-test06 \
	libkeymap-test07 \
	libkeymap-test08 \
	libkeymap-test09 \
	libkeymap-test10 \
	libkeymap-test11 \
	libkeymap-test12 \
	libkeymap-test13 \
	libkeymap-test14 \
	libkeymap-test15 \
	libkeymap-test16 \
	libkeymap-test17 \
	libkeymap-test18 \
	libkeymap-test19 \
	libkeymap-test20 \
	libkeymap-test21 \
	libkeymap-test22 \
	libkeymap-test23 \
	libkeymap-test24 \
	libkeymap-test25 \
	libkeymap-test26 \
	libkeymap-test27 \
	libkeymap-test28 \
	libkeymap-test29 \
	libkeymap-test30 \
	libkeymap-test31 \
	libkeymap-test32 \
	libkeymap-test43 \
	libkeymap-test44 \
	libkeymap-test50 \
	libkeymap-test51 \
	libkeymap-test52 \
	libkeymap-test53 \
	$(NULL)

if USE_XKB
noinst_PROGRAMS += libkeymap-test54
noinst_PROGRAMS += libkeymap-test55
noinst_PROGRAMS += libkeymap-test56
noinst_PROGRAMS += libkeymap-test57
noinst_PROGRAMS += libkeymap-test58
noinst_PROGRAMS += libkeymap-test59
noinst_PROGRAMS += libkeymap-test60

libkeymap_test54_SOURCES = \
	libkeymap-test54.c \
	$(top_srcdir)/src/xkbsupport.c

libkeymap_test54_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-DDATADIR=\"$(realpath $(top_srcdir))/data\" \
	-I$(top_srcdir)/src

libkeymap_test54_LDADD = \
	$(LDADD) \
	@XKBCOMMON_LIBS@

libkeymap_test55_SOURCES = \
	libkeymap-test55.c

libkeymap_test55_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-DDATADIR=\"$(realpath $(top_srcdir))/data\" \
	-I$(top_srcdir)/src

libkeymap_test55_LDADD = \
	$(LDADD) \
	@XKBCOMMON_LIBS@

libkeymap_test56_SOURCES = \
	libkeymap-test56.c \
	$(top_srcdir)/src/xkbsupport.c

libkeymap_test56_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-DDATADIR=\"$(realpath $(top_srcdir))/data\" \
	-I$(top_srcdir)/src

libkeymap_test56_LDADD = \
	$(LDADD) \
	@XKBCOMMON_LIBS@

libkeymap_test57_SOURCES = \
	libkeymap-test57.c \
	$(top_srcdir)/src/xkbsupport.c

libkeymap_test57_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-DDATADIR=\"$(realpath $(top_srcdir))/data\" \
	-I$(top_srcdir)/src

libkeymap_test57_LDADD = \
	$(LDADD) \
	@XKBCOMMON_LIBS@

libkeymap_test58_SOURCES = \
	libkeymap-test58.c \
	$(top_srcdir)/src/xkbsupport.c

libkeymap_test58_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-DDATADIR=\"$(realpath $(top_srcdir))/data\" \
	-I$(top_srcdir)/src

libkeymap_test58_LDADD = \
	$(LDADD) \
	@XKBCOMMON_LIBS@

libkeymap_test59_SOURCES = \
	libkeymap-test59.c \
	$(top_srcdir)/src/xkbsupport.c

libkeymap_test59_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-DDATADIR=\"$(realpath $(top_srcdir))/data\" \
	-I$(top_srcdir)/src

libkeymap_test59_LDADD = \
	$(LDADD) \
	@XKBCOMMON_LIBS@

libkeymap_test60_SOURCES = \
	libkeymap-test60.c \
	$(top_srcdir)/src/xkbsupport.c

libkeymap_test60_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-DDATADIR=\"$(realpath $(top_srcdir))/data\" \
	-I$(top_srcdir)/src

libkeymap_test60_LDADD = \
	$(LDADD) \
	@XKBCOMMON_LIBS@
endif

libkeymap_test44_SOURCES = \
	libkeymap-test44.c \
	$(top_srcdir)/src/libkeymap/ksyms.c

libkeymap_test44_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_builddir)/src/libkeymap
