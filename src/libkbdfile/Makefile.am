include $(top_srcdir)/Makefile.common

AM_CPPFLAGS += -I$(srcdir)
LDADD += $(top_builddir)/src/libcommon/libcommon.a

headers = \
	../include/kbd/compiler_attributes.h \
	../include/kbdfile.h \
	../include/kbd/kbdfile.h

libkbdfile_la_SOURCES = \
	$(headers) \
	contextP.h \
	init.c \
	kbdfile.c

KBDFILE_CURRENT  = 1
KBDFILE_REVISION = 0
KBDFILE_AGE      = 0

if BUILD_LIBKBDFILE
lib_LTLIBRARIES = libkbdfile.la
nobase_include_HEADERS = $(headers)
libkbdfile_la_LDFLAGS = -export-symbols-regex ^kbdfile_.* -version-info $(KBDFILE_CURRENT):$(KBDFILE_REVISION):$(KBDFILE_AGE)
else
noinst_LTLIBRARIES = libkbdfile.la
endif
