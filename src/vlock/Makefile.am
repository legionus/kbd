include $(top_srcdir)/Makefile.common

EXTRA_DIST = README.vlock vlock.pamd

if VLOCK
bin_PROGRAMS = vlock

install-data-local:
	install -Dm644 $(srcdir)/vlock.pamd $(DESTDIR)$(sysconfdir)/pam.d/vlock
uninstall-local:
	rm $(DESTDIR)$(sysconfdir)/pam.d/vlock
endif

vlock_SOURCES  = \
	auth.c \
	pam_auth.h \
	pam.c \
	parse.c \
	screen.c \
	username.c \
	vlock.c \
	vlock.h \
	vt.c

vlock_CFLAGS = @PAM_CFLAGS@ @PAM_MISC_CFLAGS@
vlock_LDADD = $(top_builddir)/src/libcommon/libcommon.a @PAM_LIBS@ @PAM_MISC_LIBS@
