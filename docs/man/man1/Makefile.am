include $(top_srcdir)/Makefile.common

gen_MANS = dumpkeys.1 loadkeys.1
dist_man_MANS = chvt.1 deallocvt.1 fgconsole.1 kbd_mode.1 openvt.1 \
		psfaddtable.1 psfgettable.1 psfstriptable.1 psfxtable.1 \
		setleds.1 setmetamode.1 showkey.1 unicode_start.1 unicode_stop.1 \
		kbdinfo.1 \
		$(gen_MANS)

if VLOCK
dist_man_MANS += vlock.1
endif

CLEANFILES = $(gen_MANS)
