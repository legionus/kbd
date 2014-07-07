#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
AC_INIT(kbd, 2.0.2, gladkov.alexey@gmail.com,, http://kbd-project.org/)

AC_PREREQ(2.60)

AC_CONFIG_AUX_DIR(config)
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.9 -Wall color-tests dist-xz])
AC_CONFIG_SRCDIR([src/loadkeys.c])
AC_CONFIG_HEADERS(config.h)

PKG_CHECK_MODULES([CHECK], [check >= 0.9.4])

m4_ifndef([AM_SILENT_RULES], [m4_define([AM_SILENT_RULES],[])])
AM_SILENT_RULES([yes])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_YACC
AC_PROG_LEX
AC_PROG_SED
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_CANONICAL_HOST

dnl older versions of automake didn't include this macro
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
LT_INIT

AM_GNU_GETTEXT_VERSION([0.14.1])
AM_GNU_GETTEXT([external])
ALL_LINGUAS=`cd $srcdir/po > /dev/null && echo *.po | sed 's/\.po//g'`

# Checks for libraries.

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([fcntl.h libintl.h locale.h memory.h stdlib.h string.h \
sys/file.h sys/ioctl.h termios.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_STRUCT_TM
AC_TYPE_UID_T

# Checks for library functions.
AC_FUNC_CHOWN
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_FORK
AC_PROG_GCC_TRADITIONAL
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_CHECK_FUNCS([alarm memset setlocale strcasecmp strchr strdup strerror \
strspn strstr strtol strtoul setpgrp malloc realloc])

CC_CHECK_CFLAGS_APPEND([\
	-Wall \
	-Wextra \
	-Wmissing-noreturn \
	-Wdisabled-optimization \
	-Wcast-align \
	-Wshadow \
	-Wmissing-format-attribute \
	-Wmissing-prototypes \
	-Wstrict-prototypes \
	-Wmissing-declarations])

case $host_cpu in
	m68*|sparc*) KEYCODES_PROGS=no ;;
	*)           KEYCODES_PROGS=yes ;;
esac
AM_CONDITIONAL(KEYCODES_PROGS, test "$KEYCODES_PROGS" = "yes")

case $host_cpu in
	i?86*) RESIZECONS_PROGS=yes ;;
	x86_64*) RESIZECONS_PROGS=yes ;;
	*)     RESIZECONS_PROGS=no ;;
esac
AM_CONDITIONAL(RESIZECONS_PROGS, test "$RESIZECONS_PROGS" = "yes")

AC_ARG_ENABLE(optional-progs,
	AS_HELP_STRING(--enable-optional-progs, [Build and install a optional programs]),
	[OPTIONAL_PROGS=$enableval],[OPTIONAL_PROGS=no])
AM_CONDITIONAL(OPTIONAL_PROGS, test "$OPTIONAL_PROGS" = "yes")

AC_ARG_ENABLE(libkeymap,
	AS_HELP_STRING(--enable-libkeymap, [Build and install a libkeymap]),
	[BUILD_LIBKEYMAP=$enableval],[BUILD_LIBKEYMAP=no])
AM_CONDITIONAL(BUILD_LIBKEYMAP, test "$BUILD_LIBKEYMAP" = "yes")

AC_ARG_ENABLE(vlock,
	AS_HELP_STRING(--disable-vlock, [do not build vlock]),
	[VLOCK_PROG=no],[VLOCK_PROG=yes])
AM_CONDITIONAL(VLOCK, test "$VLOCK_PROG" = "yes")

if test "$VLOCK_PROG" = "yes"; then
	AC_CHECK_LIB(pam, pam_start, [
                AC_CHECK_HEADER([security/pam_appl.h],
				[have_pam=1],
                                AC_MSG_ERROR([Can't find required header files.]))])
	AC_CHECK_LIB(pam_misc, misc_conv, [
                AC_CHECK_HEADER([security/pam_misc.h],
				[have_pam_misc=1],
                                AC_MSG_ERROR([Can't find required header files.]))])
	if test -z "$have_pam" -o -z "$have_pam_misc"; then
		AC_MSG_ERROR([libpam-devel required.])
	fi
	AC_SUBST(PAM_LIBS, "-lpam -lpam_misc")
fi

AC_CHECK_PROG([HAVE_DOXYGEN], [doxygen], [yes], [no])
AM_CONDITIONAL(HAVE_DOXYGEN, test "$HAVE_DOXYGEN" = "yes")

AC_CONFIG_FILES([Makefile
		data/Makefile
		docs/Makefile
		docs/man/Makefile
		docs/man/man1/Makefile
		docs/man/man1/dumpkeys.1
		docs/man/man1/loadkeys.1
		docs/man/man1misc/Makefile
		docs/man/man5/Makefile
		docs/man/man8/Makefile
		docs/man/man8/loadunimap.8
		docs/man/man8/mapscrn.8
		docs/man/man8/setfont.8
		docs/man/man8misc/Makefile
		docs/reference/Makefile
		docs/reference/libkeymap/Makefile
		po/Makefile.in
		src/Makefile
		src/libkeymap/Makefile
		src/vlock/Makefile
		tests/Makefile])
AC_OUTPUT

AC_MSG_RESULT([
	$PACKAGE $VERSION
	======

	prefix:			${prefix}
	libdir:			${libdir}
	bindir:			${bindir}
	datadir:		${datadir}

	compiler:		${CC}
	cflags:			${CFLAGS}

	{get,set}keycodes:	${KEYCODES_PROGS}
	resizecons:		${RESIZECONS_PROGS}
	optional progs:		${OPTIONAL_PROGS}
	vlock:			${VLOCK_PROG}
	libkeymap:		${BUILD_LIBKEYMAP}
])
