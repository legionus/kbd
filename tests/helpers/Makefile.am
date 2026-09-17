NULL =

AM_CPPFLAGS = \
	$(CODE_COVERAGE_CPPFLAGS) \
	-I$(top_srcdir)/src/include \
	-I$(top_srcdir)/src/libcommon \
	-I$(top_srcdir)/src/libkeymap

AM_CFLAGS = $(CHECK_CFLAGS) $(CODE_COVERAGE_CFLAGS)
AM_LDFLAGS = $(CODE_COVERAGE_CFLAGS)

LDADD  = \
	$(top_builddir)/src/libcommon/libcommon.a \
	$(top_builddir)/src/libkeymap/libkeymap.la \
	$(top_builddir)/src/libkbdfile/libkbdfile.la \
	@LIBINTL@ $(CODE_COVERAGE_LIBS)

noinst_PROGRAMS = \
	showkey-test      \
	libkeymap-bkeymap  \
	libkeymap-dumpkeys \
	libkeymap-mktable  \
	libkeymap-showmaps \
	$(NULL)

showkey_test_CPPFLAGS = $(AM_CPPFLAGS) -D_GNU_SOURCE
showkey_test_LDADD = $(top_builddir)/src/libcommon/libcommon.a @LIBINTL@ $(CODE_COVERAGE_LIBS)
