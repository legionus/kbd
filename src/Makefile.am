AM_CPPFLAGS = \
	-I$(srcdir)/libkeymap -I$(builddir)/libkeymap \
	-I$(srcdir)/libkeymap/keymap -I$(builddir)/libkeymap/keymap \
	-DDATADIR=\"$(datadir)\" -DLOCALEDIR=\"$(localedir)\" \
	-D_FORTIFY_SOURCE=2 \
	-funit-at-a-time

SUBDIRS = vlock libkeymap

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


libcommon_a_SOURCES = \
	getfd.c getfd.h \
	xmalloc.c xmalloc.h

libfont_a_SOURCES   = \
	psf.h psffontop.c psffontop.h \
	utf8.c utf8.h \
	kdmapop.c kdmapop.h \
	loadunimap.c loadunimap.h \
	kdfontop.c kdfontop.h

ALL_S   = nls.h paths.h version.h kbd.h

chvt_SOURCES            = $(ALL_S) chvt.c
clrunimap_SOURCES       = $(ALL_S) clrunimap.c
deallocvt_SOURCES       = $(ALL_S) deallocvt.c
fgconsole_SOURCES       = $(ALL_S) fgconsole.c
getkeycodes_SOURCES     = $(ALL_S) getkeycodes.c
getunimap_SOURCES       = $(ALL_S) getunimap.c
kbd_mode_SOURCES        = $(ALL_S) kbd_mode.c
loadunimap_SOURCES      = $(ALL_S) loadunimap.c
mapscrn_SOURCES         = $(ALL_S) mapscrn.c
openvt_SOURCES          = $(ALL_S) openvt.c
psfxtable_SOURCES       = $(ALL_S) psfxtable.c
resizecons_SOURCES      = $(ALL_S) resizecons.c
screendump_SOURCES      = $(ALL_S) screendump.c
setfont_SOURCES         = $(ALL_S) setfont.c mapscrn.c
setkeycodes_SOURCES     = $(ALL_S) setkeycodes.c
setvtrgb_SOURCES        = $(ALL_S) setvtrgb.c
setlogcons_SOURCES      = $(ALL_S) setlogcons.c
setpalette_SOURCES      = $(ALL_S) setpalette.c
setvesablank_SOURCES    = $(ALL_S) setvesablank.c
showconsolefont_SOURCES = $(ALL_S) showconsolefont.c
showkey_SOURCES         = $(ALL_S) showkey.c
kbdinfo_SOURCES         = $(ALL_S) kbdinfo.c

dumpkeys_SOURCES        = $(ALL_S) dumpkeys.c
loadkeys_SOURCES        = $(ALL_S) loadkeys.c

mapscrn_CFLAGS = -DMAIN
loadunimap_CFLAGS = -DMAIN

noinst_LIBRARIES = libcommon.a libfont.a
LDADD = libcommon.a libfont.a libkeymap/libkeymap.la

install-exec-hook:
	for i in psfaddtable psfgettable psfstriptable; do \
		rm -f $(DESTDIR)$(bindir)/$$i; \
		ln -s psfxtable $(DESTDIR)$(bindir)/$$i || exit $$?; \
	done
