AM_CPPFLAGS = $(CODE_COVERAGE_CPPFLAGS) -I$(srcdir) -I$(builddir)
AM_CFLAGS   = $(CODE_COVERAGE_CFLAGS) -g -O0
LDADD       = $(CODE_COVERAGE_LIBS)

LIBTSWRAP_CURRENT  = 1
LIBTSWRAP_REVISION = 0
LIBTSWRAP_AGE      = 0

libtswrap_la_SOURCES = ioctl.c
libtswrap_la_LIBADD  = -ldl
libtswrap_la_LDFLAGS = -export-symbols-regex ^ioctl -version-info $(LIBTSWRAP_CURRENT):$(LIBTSWRAP_REVISION):$(LIBTSWRAP_AGE)

lib_LTLIBRARIES    = libtswrap.la
