AM_CPPFLAGS = -I.. -D_GNU_SOURCE -DLOCALEDIR=\"$(localedir)\"
AM_CFLAGS = -pipe -O2

if VLOCK
bin_PROGRAMS = vlock
EXTRA_DIST = README.vlock
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
