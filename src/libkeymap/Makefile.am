include $(top_srcdir)/Makefile.common

CLEANFILES = libkeymap.pc
EXTRA_DIST = libkeymap.pc.gen

LK_CURRENT  = 1
LK_REVISION = 0
LK_AGE      = 0

headers = \
	../include/kbd/compiler_attributes.h \
	../include/keymap.h \
	../include/kbd/keymap/array.h \
	../include/kbd/keymap/context.h \
	../include/kbd/keymap/charset.h \
	../include/kbd/keymap/common.h \
	../include/kbd/keymap/dump.h \
	../include/kbd/keymap/kernel.h \
	../include/kbd/keymap/kmap.h \
	../include/kbd/keymap/logging.h

AM_CPPFLAGS += -I$(srcdir)

SYMS = \
  syms.latin1.h/8859-1/160 \
  syms.latin2.h/8859-2/160 \
  syms.latin3.h/8859-3/160 \
  syms.latin4.h/8859-4/160 \
  syms.iso8859_5.h/8859-5/160 \
  syms.iso8859_7.h/8859-7/160 \
  syms.iso8859_8.h/8859-8/160 \
  syms.iso8859_9.h/8859-9/160 \
  syms.iso8859_15.h/8859-15/160 \
  syms.cp1250.h/cp1250/128 \
  syms.koi8.h/koi8-u/128

ksyms_gen_headers = $(call charset_headers)
ksyms_headers = \
	$(ksyms_gen_headers) \
	syms.ktyp.h \
	syms.synonyms.h \
	syms.cp1250.h \
	syms.ethiopic.h \
	syms.mazovia.h \
	syms.sami.h \
	syms.thai.h

libkeymap_la_SOURCES = \
	$(headers) \
	array.c \
	common.c kernel.c dump.c kmap.c diacr.c func.c summary.c loadkeys.c \
	contextP.h \
	parser.y parser.h analyze.l analyze.h \
	modifiers.c modifiers.h \
	ksyms.c ksyms.h \
	$(ksyms_headers)

libkeymap_la_LIBADD = $(builddir)/../libkbdfile/libkbdfile.la

if BUILD_LIBKEYMAP
pkgconfigdir   = $(libdir)/pkgconfig
pkgconfig_DATA = libkeymap.pc

lib_LTLIBRARIES = libkeymap.la
nobase_include_HEADERS = $(headers)
libkeymap_la_LDFLAGS = -export-symbols-regex ^lk_.* -version-info $(LK_CURRENT):$(LK_REVISION):$(LK_AGE)
else
noinst_LTLIBRARIES = libkeymap.la
endif

AM_YFLAGS = -Wno-yacc
AM_LFLAGS = --header-file=$(CURDIR)/analyze.h
BUILT_SOURCES = parser.c analyze.c parser.h analyze.h $(ksyms_gen_headers)
CLEANFILES += $(BUILT_SOURCES)

TRANS_DIR = $(top_srcdir)/data/consoletrans

GEN_SYMS     = $(top_srcdir)/contrib/gen_charset_syms.pl
V_GEN_SYMS   = $(V_GEN_SYMS_$(V))
V_GEN_SYMS_  = $(V_GEN_SYMS_$(AM_DEFAULT_VERBOSITY))
V_GEN_SYMS_0 = @echo "  CHARSET " $@;
V_GEN_SYMS_1 =

charset_headers = $(foreach x,$(SYMS),$(word 1,$(subst /, ,$(x))))
charset_trans   = $(TRANS_DIR)/$(word 2,$(subst /, ,$(filter syms.$(1).h/%,$(SYMS))))_to_uni.trans
charset_start   = $(word 3,$(subst /, ,$(filter syms.$(1).h/%,$(SYMS))))

parser.h: parser.c ; @true
analyze.h: analyze.c ; @true

syms.%.h:
	$(V_GEN_SYMS)$(GEN_SYMS) -n $*_syms -s $(call charset_start,$*) <$(call charset_trans,$*) >$(builddir)/$@
