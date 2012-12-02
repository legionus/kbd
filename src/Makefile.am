AM_CPPFLAGS = -DDATADIR=\"$(datadir)\" -DLOCALEDIR=\"$(localedir)\" \
	-funit-at-a-time

SUBDIRS = vlock

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
	loadkeys.analyze.c loadkeys.analyze.h loadkeys.analyze.l wctomb.c \
	totextmode.c README.cad README.init README.openvt \
	cad

ALL_S   = modifiers.h nls.h paths.h version.h kbd.h
FIND_S  = findfile.c findfile.h
GETFD_S = getfd.c getfd.h
KDMA_S  = kdmapop.c kdmapop.h
KSYMS_S = ksyms.c ksyms.h cp1250.syms.h ethiopic.syms.h koi8.syms.h sami.syms.h thai.syms.h
PSF_S   = psf.h psffontop.c psffontop.h
UNIM_S  = loadunimap.c loadunimap.h
UTF8_S  = utf8.c utf8.h
XMAL_S  = xmalloc.c xmalloc.h

chvt_SOURCES            = $(ALL_S) chvt.c $(GETFD_S)
clrunimap_SOURCES       = $(ALL_S) clrunimap.c $(GETFD_S) $(KDMA_S)
deallocvt_SOURCES       = $(ALL_S) deallocvt.c $(GETFD_S)
dumpkeys_SOURCES        = $(ALL_S) dumpkeys.c $(GETFD_S) $(KSYMS_S) $(XMAL_S)
fgconsole_SOURCES       = $(ALL_S) fgconsole.c $(GETFD_S)
getkeycodes_SOURCES     = $(ALL_S) getkeycodes.c $(GETFD_S)
getunimap_SOURCES       = $(ALL_S) getunimap.c $(GETFD_S) $(KDMA_S)
kbd_mode_SOURCES        = $(ALL_S) kbd_mode.c $(GETFD_S)
loadunimap_SOURCES      = $(ALL_S) loadunimap.c $(FIND_S) $(PSF_S) $(UTF8_S) $(XMAL_S) $(GETFD_S) $(KDMA_S)
mapscrn_SOURCES         = $(ALL_S) mapscrn.c $(FIND_S) $(UTF8_S) $(XMAL_S) $(GETFD_S) $(KDMA_S)
openvt_SOURCES          = $(ALL_S) openvt.c $(GETFD_S) $(XMAL_S)
psfxtable_SOURCES       = $(ALL_S) psfxtable.c $(PSF_S) $(XMAL_S) $(UTF8_S) $(GETFD_S)
resizecons_SOURCES      = $(ALL_S) resizecons.c $(GETFD_S) $(FIND_S) $(XMAL_S)
screendump_SOURCES      = $(ALL_S) screendump.c $(XMAL_S)
setfont_SOURCES         = $(ALL_S) setfont.c $(FIND_S) $(XMAL_S) $(GETFD_S) mapscrn.c $(KDMA_S) $(PSF_S) $(UTF8_S) kdfontop.c kdfontop.h $(UNIM_S)
setkeycodes_SOURCES     = $(ALL_S) setkeycodes.c $(GETFD_S)
setvtrgb_SOURCES        = $(ALL_S) setvtrgb.c $(GETFD_S)
setlogcons_SOURCES      = $(ALL_S) setlogcons.c $(GETFD_S)
setpalette_SOURCES      = $(ALL_S) setpalette.c $(GETFD_S)
setvesablank_SOURCES    = $(ALL_S) setvesablank.c $(GETFD_S)
showconsolefont_SOURCES = $(ALL_S) showconsolefont.c $(GETFD_S) $(XMAL_S) $(KDMA_S) kdfontop.c kdfontop.h
showkey_SOURCES         = $(ALL_S) showkey.c $(GETFD_S)
kbdinfo_SOURCES         = $(ALL_S) kbdinfo.c $(GETFD_S)

mapscrn_CFLAGS = -DMAIN
loadunimap_CFLAGS = -DMAIN

AM_YFLAGS = -d
AM_LFLAGS = --header-file=$(CURDIR)/loadkeys.analyze.h
BUILT_SOURCES = loadkeys.c loadkeys.analyze.c loadkeys.h loadkeys.analyze.h
loadkeys_SOURCES = loadkeys.y loadkeys.analyze.l $(GETFD_S) $(KSYMS_S) $(XMAL_S) $(FIND_S)
loadkeys.analyze.h: loadkeys.analyze.c ; @true

install-exec-hook:
	for i in psfaddtable psfgettable psfstriptable; do \
		rm -f $(DESTDIR)$(bindir)/$$i; \
		ln -s psfxtable $(DESTDIR)$(bindir)/$$i || exit $$?; \
	done
