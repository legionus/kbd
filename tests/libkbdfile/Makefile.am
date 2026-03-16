NULL =

AM_CPPFLAGS = \
	$(CODE_COVERAGE_CPPFLAGS) \
	-I$(top_srcdir)/src/include \
	-I$(top_srcdir)/src/libcommon \
	-I$(top_srcdir)/src/libkbdfile \
	-DTESTDIR=\"$(realpath $(top_srcdir))/tests\"

AM_CFLAGS = $(CHECK_CFLAGS) $(CODE_COVERAGE_CFLAGS)
AM_LDFLAGS = $(CODE_COVERAGE_CFLAGS)

LDADD  = \
	$(top_builddir)/src/libcommon/libcommon.a \
	$(top_builddir)/src/libkbdfile/libkbdfile.la \
	@LIBINTL@ $(CODE_COVERAGE_LIBS)

noinst_HEADERS = libkbdfile-test.h

noinst_PROGRAMS = \
	libkbdfile-test01 \
	libkbdfile-test02 \
	libkbdfile-test03 \
	libkbdfile-test14 \
	libkbdfile-test16 \
	$(NULL)

libkbdfile_test16_SOURCES = \
	libkbdfile-test16.c \
	$(top_srcdir)/src/libkbdfile/elf-note.c

libkbdfile_test16_LDADD = \
	$(top_builddir)/src/libcommon/libcommon.a \
	@LIBINTL@ $(CODE_COVERAGE_LIBS)
