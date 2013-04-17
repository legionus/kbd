AM_CPPFLAGS = -I$(srcdir)/.. -I$(builddir)/.. -D_GNU_SOURCE -DLOCALEDIR=\"$(localedir)\"

EXTRA_DIST = README.vlock

if VLOCK
bin_PROGRAMS = vlock
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

vlock_LDADD = @PAM_LIBS@
