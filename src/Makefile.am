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

chvt_SOURCES            = chvt.c
clrunimap_SOURCES       = clrunimap.c
deallocvt_SOURCES       = deallocvt.c
fgconsole_SOURCES       = fgconsole.c
getkeycodes_SOURCES     = getkeycodes.c
getunimap_SOURCES       = getunimap.c
kbd_mode_SOURCES        = kbd_mode.c
loadunimap_SOURCES      = loadunimap.c
mapscrn_SOURCES         = mapscrn.c
openvt_SOURCES          = openvt.c
psfxtable_SOURCES       = psfxtable.c
resizecons_SOURCES      = resizecons.c
screendump_SOURCES      = screendump.c
setfont_SOURCES         = setfont.c
setkeycodes_SOURCES     = setkeycodes.c
setvtrgb_SOURCES        = setvtrgb.c
setlogcons_SOURCES      = setlogcons.c
setpalette_SOURCES      = setpalette.c
setvesablank_SOURCES    = setvesablank.c
showconsolefont_SOURCES = showconsolefont.c
showkey_SOURCES         = showkey.c
kbdinfo_SOURCES         = kbdinfo.c
dumpkeys_SOURCES        = dumpkeys.c
loadkeys_SOURCES        = loadkeys.c

getunimap_CFLAGS = -DUSE_LIBC

spawn_console_SOURCES = spawn_command.c
spawn_login_SOURCES   = spawn_command.c

spawn_console_CFLAGS = -DSPAWN_CONSOLE
spawn_login_CFLAGS   = -DSPAWN_LOGIN

LDADD += \
	libkfont/libkfont.la \
	libkeymap/libkeymap.la \
	libkbdfile/libkbdfile.la \
	libcommon/libcommon.a \
	@LIBINTL@

install-exec-hook:
	for i in psfaddtable psfgettable psfstriptable; do \
		rm -f $(DESTDIR)$(bindir)/$$i; \
		$(LN_S) psfxtable $(DESTDIR)$(bindir)/$$i || exit $$?; \
	done
