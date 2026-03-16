NULL =

AM_CPPFLAGS = \
	$(CODE_COVERAGE_CPPFLAGS) \
	-I$(top_srcdir)/src/include \
	-I$(top_srcdir)/src/libcommon \
	-I$(top_srcdir)/src/libkfont \
	-DTESTDIR=\"$(realpath $(top_srcdir))/tests\"

AM_CFLAGS = $(CHECK_CFLAGS) $(CODE_COVERAGE_CFLAGS)
AM_LDFLAGS = $(CODE_COVERAGE_CFLAGS)

LDADD  = \
	$(top_builddir)/src/libcommon/libcommon.a \
	$(top_builddir)/src/libkfont/libkfont.la \
	@LIBINTL@ $(CODE_COVERAGE_LIBS)

noinst_PROGRAMS = \
	libkfont-test01 \
	libkfont-test05 \
	libkfont-test08 \
	libkfont-test13 \
	libkfont-test14 \
	libkfont-test15 \
	libkfont-test16 \
	libkfont-test17 \
	libkfont-test18 \
	libkfont-test19 \
	libkfont-test24 \
	$(NULL)

libkfont_test16_SOURCES = \
	libkfont-test16.c \
	$(top_srcdir)/src/libkfont/context.c \
	$(top_srcdir)/src/libkfont/psffontop.c \
	$(top_srcdir)/src/libkfont/unicode.c \
	$(top_srcdir)/src/libkfont/utf8.c

libkfont_test16_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-DDATADIR=\".\"

libkfont_test16_LDADD = \
	$(top_builddir)/src/libcommon/libcommon.a \
	@LIBINTL@ $(CODE_COVERAGE_LIBS)

libkfont_test17_SOURCES = \
	libkfont-test17.c \
	$(top_srcdir)/src/libkfont/context.c \
	$(top_srcdir)/src/libkfont/psffontop.c \
	$(top_srcdir)/src/libkfont/unicode.c \
	$(top_srcdir)/src/libkfont/utf8.c

libkfont_test17_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-DDATADIR=\".\"

libkfont_test17_LDADD = \
	$(top_builddir)/src/libcommon/libcommon.a \
	@LIBINTL@ $(CODE_COVERAGE_LIBS)

libkfont_test24_SOURCES = \
	libkfont-test24.c \
	$(top_srcdir)/src/libkfont/context.c \
	$(top_srcdir)/src/libkfont/kdfontop.c

libkfont_test24_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-DDATADIR=\".\"

libkfont_test24_LDADD = \
	$(top_builddir)/src/libcommon/libcommon.a \
	$(top_builddir)/src/libkfont/libkfont.la \
	@LIBINTL@ $(CODE_COVERAGE_LIBS)
