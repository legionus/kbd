WARNINGS = -Wall -Wextra -Wmissing-noreturn -Wdisabled-optimization \
	-Wcast-align -Wshadow -Wmissing-format-attribute -Wmissing-prototypes \
	-Wstrict-prototypes -Wmissing-declarations

AM_CPPFLAGS = -D_GNU_SOURCE
AM_CFLAGS = -pipe $(WARNINGS) -O2

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
