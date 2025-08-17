include $(top_srcdir)/Makefile.common

AM_CPPFLAGS += -I$(srcdir)

headers = \
	../include/kbd/compiler_attributes.h \
	../include/kbdfile.h \
	../include/kbd/kbdfile.h

libkbdfile_la_SOURCES = \
	$(headers) \
	contextP.h \
	elf-note.h \
	elf-note.c \
	init.c \
	kbdfile.c

libkbdfile_la_LIBADD =
libkbdfile_la_CFLAGS =

if USE_ZLIB
libkbdfile_la_SOURCES += kbdfile-zlib.c
libkbdfile_la_CFLAGS += $(ZLIB_CFLAGS)
endif

if USE_BZIP2
libkbdfile_la_SOURCES += kbdfile-bzip2.c
libkbdfile_la_CFLAGS += $(BZIP2_CFLAGS)
endif

if USE_LZMA
libkbdfile_la_SOURCES += kbdfile-lzma.c
libkbdfile_la_CFLAGS += $(LZMA_CFLAGS)
endif

if USE_ZSTD
libkbdfile_la_SOURCES += kbdfile-zstd.c
libkbdfile_la_CFLAGS += $(ZSTD_CFLAGS)
endif

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
