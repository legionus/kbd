include $(top_srcdir)/Makefile.common

SUBDIRS = libcommon libkbdfile libkfont libkeymap vlock

OLDPROGS = mapscrn loadunimap
PROGS = \
	dumpkeys loadkeys showkey setfont showconsolefont \
	setleds setmetamode kbd_mode psfxtable fgconsole \
	kbdrate chvt deallocvt openvt kbdinfo setvtrgb

if KEYCODES_PROGS
PROGS += getkeycodes setkeycodes
endif

if RESIZECONS_PROGS
PROGS += resizecons
endif

if OPTIONAL_PROGS
PROGS += \
	setlogcons setvesablank getunimap clrunimap outpsfheader setpalette \
	screendump spawn_console spawn_login
endif

dist_bin_SCRIPTS = unicode_start unicode_stop
bin_PROGRAMS = $(OLDPROGS) $(PROGS)

EXTRA_DIST = \
	wctomb.c \
	totextmode.c README.cad README.init README.openvt \
	cad

getunimap_CFLAGS = -DUSE_LIBC

spawn_console_SOURCES = spawn_command.c
spawn_login_SOURCES   = spawn_command.c

spawn_console_CFLAGS = -DSPAWN_CONSOLE
spawn_login_CFLAGS   = -DSPAWN_LOGIN

COMMON_LIBS = \
	libcommon/libcommon.a \
	@INTLLIBS@

chvt_LDADD            = $(COMMON_LIBS)
deallocvt_LDADD       = $(COMMON_LIBS)
fgconsole_LDADD       = $(COMMON_LIBS)
getkeycodes_LDADD     = $(COMMON_LIBS)
kbd_mode_LDADD        = $(COMMON_LIBS)
kbdinfo_LDADD         = $(COMMON_LIBS)
kbdrate_LDADD         = $(COMMON_LIBS)
openvt_LDADD          = $(COMMON_LIBS)
screendump_LDADD      = $(COMMON_LIBS)
setkeycodes_LDADD     = $(COMMON_LIBS)
setleds_LDADD         = $(COMMON_LIBS)
setlogcons_LDADD      = $(COMMON_LIBS)
setmetamode_LDADD     = $(COMMON_LIBS)
setpalette_LDADD      = $(COMMON_LIBS)
setvesablank_LDADD    = $(COMMON_LIBS)
setvtrgb_LDADD        = $(COMMON_LIBS)
showkey_LDADD         = $(COMMON_LIBS)
spawn_console_LDADD   = $(COMMON_LIBS)
spawn_login_LDADD     = $(COMMON_LIBS)
outpsfheader_LDADD    = $(COMMON_LIBS)

resizecons_LDADD      = libkbdfile/libkbdfile.la $(COMMON_LIBS)

clrunimap_LDADD       = libkfont/libkfont.la $(COMMON_LIBS)
getunimap_LDADD       = libkfont/libkfont.la $(COMMON_LIBS)
loadunimap_LDADD      = libkfont/libkfont.la $(COMMON_LIBS)
mapscrn_LDADD         = libkfont/libkfont.la $(COMMON_LIBS)
psfxtable_LDADD       = libkfont/libkfont.la $(COMMON_LIBS)
setfont_LDADD         = libkfont/libkfont.la $(COMMON_LIBS)
showconsolefont_LDADD = libkfont/libkfont.la $(COMMON_LIBS)

dumpkeys_LDADD        = libkeymap/libkeymap.la libkbdfile/libkbdfile.la $(COMMON_LIBS)
loadkeys_LDADD        = libkeymap/libkeymap.la libkbdfile/libkbdfile.la $(COMMON_LIBS)

install-exec-hook:
	for i in psfaddtable psfgettable psfstriptable; do \
		rm -f $(DESTDIR)$(bindir)/$$i; \
		$(LN_S) psfxtable $(DESTDIR)$(bindir)/$$i || exit $$?; \
	done
