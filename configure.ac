#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(kbd, 1.14, legion@altlinux.org)
AC_CONFIG_SRCDIR([src/showkey.c])
AC_CONFIG_HEADERS(config.h)

# Checks for programs.
AC_PROG_YACC
AC_PROG_CC
AC_PROG_LEX
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

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
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_SETPGRP
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_CHECK_FUNCS([alarm memset setlocale strcasecmp strchr strdup strerror \
strspn strstr strtol strtoul])

OPTIONAL_PROGS=
AC_ARG_ENABLE(optional-progs,
	AS_HELP_STRING(--enable-optional-progs, [Build and install a optional programs]),
	[OPTIONAL_PROGS=1])
AC_SUBST(OPTIONAL_PROGS)

# For lib/nls.h: do we have <libintl.h> and gettext() ?

AC_ARG_ENABLE(nls, AS_HELP_STRING(--enable-nls, [NLS support]),
	[enable_nls=1], [enable_nls=])

if test "$enable_nls" = "1"; then
	AC_DEFINE(ENABLE_NLS)
fi

# Does xgettext exist and take the option --foreign-user?
FOREIGN=
HAVE_XGETTEXT=no
if (test "$enable_nls" = "1" && type xgettext > /dev/null 2>&1); then
	msg=`xgettext --foreign-user 2>&1 | grep unrecognized`
	if test -z "$msg"; then
		FOREIGN="--foreign-user"
	fi
	HAVE_XGETTEXT=yes
fi
AC_SUBST(FOREIGN)
AC_SUBST(HAVE_XGETTEXT)

AC_CONFIG_FILES([Makefile
		data/Makefile
		man/Makefile
		po/Makefile
		src/Makefile])
AC_OUTPUT
