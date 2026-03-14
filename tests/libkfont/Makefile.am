NULL =

AM_CPPFLAGS = \
	$(CODE_COVERAGE_CPPFLAGS) \
	-I$(top_srcdir)/src/include \
	-I$(top_srcdir)/src/libcommon \
	-I$(top_srcdir)/src/libkfont \
	-DTESTDIR=\"$(realpath $(top_srcdir))/tests\"

AM_CFLAGS = $(CHECK_CFLAGS) $(CODE_COVERAGE_CFLAGS)

LDADD  = \
	$(top_builddir)/src/libcommon/libcommon.a \
	$(top_builddir)/src/libkfont/libkfont.la \
	@LIBINTL@ $(CODE_COVERAGE_LIBS)

noinst_PROGRAMS = \
	libkfont-test01 \
	libkfont-test02 \
	libkfont-test03 \
	libkfont-test04 \
	$(NULL)
