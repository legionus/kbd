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
