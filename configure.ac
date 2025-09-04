# SPDX-License-Identifier: GPL-2.0-or-later
#
# Process this file with autoconf to produce a configure script.
#
AC_PREREQ([2.71])

AC_INIT([kbd],[2.9.0],[gladkov.alexey@gmail.com],[kbd],[https://kbd-project.org/])

AC_CONFIG_AUX_DIR(config)
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.9 -Wall -Wno-portability color-tests tar-pax dist-xz])
AC_CONFIG_SRCDIR([src/loadkeys.c])
AC_CONFIG_TESTDIR([tests])
AC_CONFIG_HEADERS([config.h])

m4_ifdef([AM_SILENT_RULES],
	[AM_SILENT_RULES([yes])],
	[AC_SUBST([AM_DEFAULT_VERBOSITY], [1])])

CFLAGS="${CFLAGS:-}"

AC_MSG_NOTICE([checking for programs...])

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_YACC
AC_PROG_SED
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_CANONICAL_HOST

AC_PROG_LEX([noyywrap])

AS_IF([test "$LEX" != ":" -a "$LEX" != "flex"], [
	AC_MSG_WARN([kbd depends on flex features, but found '$LEX'.])
])

dnl older versions of automake didn't include this macro
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

AC_MSG_NOTICE([checking for libtool...])

LT_INIT([dlopen])

AC_MSG_NOTICE([checking for gettext...])

AM_GNU_GETTEXT_VERSION([0.19.8])
AM_GNU_GETTEXT([external])
ALL_LINGUAS=`cd $srcdir/po > /dev/null && echo *.po | sed 's/\.po//g'`

AC_MSG_NOTICE([checking for header files...])

AC_CHECK_HEADERS([fcntl.h inttypes.h libintl.h locale.h malloc.h stdint.h \
		  sys/file.h sys/ioctl.h sys/param.h \
		  syslog.h termios.h unistd.h])

AC_MSG_NOTICE([checking for typedefs, structures, and compiler characteristics...])

AC_C_CONST
AC_C_INLINE
AC_C_TYPEOF
AC_C__GENERIC

AC_TYPE_INT32_T
AC_TYPE_INT8_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UID_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

AC_CHECK_MEMBERS([struct stat.st_rdev])

AC_MSG_NOTICE([checking for library functions...])

AC_FUNC_CHOWN
AC_FUNC_FORK
AC_FUNC_STAT
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STRERROR_R
AC_CHECK_FUNCS([alarm dup2 endpwent memset setlocale strcasecmp strerror \
		strdup strndup strchr strrchr strspn strstr strtol])

AC_SEARCH_LIBS([timer_create], [rt])

AC_MSG_NOTICE([checking for code coverage support...])

AX_CODE_COVERAGE

AC_MSG_NOTICE([checking compiler extra flags...])

AX_CHECK_COMPILE_FLAG([-std=gnu11],[CFLAGS="$CFLAGS -std=gnu11"])

CC_O_LEVEL="-O2"
AX_ADD_FORTIFY_SOURCE

AS_IF([test "$enable_code_coverage" = "yes"], [
	CC_O_LEVEL="-O0"
	FORTIFY_SOURCE="-U_FORTIFY_SOURCE"
])
AS_CASE(["$GCC,$ac_cv_prog_cc_g"],
	[yes,yes], [CFLAGS="$CC_O_LEVEL $FORTIFY_SOURCE $CFLAGS"],
	[yes,   ], [CFLAGS="$CC_O_LEVEL $FORTIFY_SOURCE $CFLAGS"],
	[   ,yes], [CFLAGS="$CFLAGS"])

CC_CHECK_CFLAGS_APPEND([ \
	-Waggregate-return \
	-Wall \
	-Wcast-align \
	-Wconversion \
	-Wdisabled-optimization \
	-Wextra \
	-Wmissing-declarations \
	-Wmissing-format-attribute \
	-Wmissing-noreturn \
	-Wmissing-prototypes \
	-Wpointer-arith \
	-Wredundant-decls \
	-Wshadow \
	-Wstrict-prototypes \
	-Wwrite-strings \
])

AC_ARG_ENABLE([werror],
	[AS_HELP_STRING([--enable-werror],
			[turn on -Werror for some kind of warnings])],
	[use_werror=$enableval],
	[use_werror=no]
)
AS_IF([test "$use_werror" = "yes"], [
	CC_CHECK_CFLAGS_APPEND([ \
		-Werror=incompatible-pointer-types \
		-Werror=discarded-qualifiers \
	])
])

AC_MSG_NOTICE([checking the necessity of architecture-dependent programs...])

AS_CASE(["$host_cpu"],
	[m68*], [KEYCODES_PROGS=no],
	[*],    [KEYCODES_PROGS=yes]
)
AM_CONDITIONAL(KEYCODES_PROGS, test "$KEYCODES_PROGS" = "yes")

AS_CASE(["$host_cpu"],
	[i?86*],   [RESIZECONS_PROGS=yes],
	[x86_64*], [RESIZECONS_PROGS=yes],
	[*],       [RESIZECONS_PROGS=no]
)
AM_CONDITIONAL(RESIZECONS_PROGS, test "$RESIZECONS_PROGS" = "yes")

AS_CASE(["$host_cpu"],
	[s390*], [
		AC_SUBST(DEFKEYMAP, "defkeymap.map")
		AC_SUBST(KERNELDIR, "/usr/src/linux/drivers/s390/char")
	],
	[*], [
		AC_SUBST(DEFKEYMAP, "defkeymap.map")
		AC_SUBST(KERNELDIR, "/usr/src/linux/drivers/tty/vt")
	])

AC_MSG_NOTICE([checking for pkg-config...])

m4_ifndef([PKG_PROG_PKG_CONFIG], [
	m4_fatal([Could not locate the pkg-config autoconf macros.

These are usually located in /usr/share/aclocal/pkg.m4. If your macros are in a
different location, try setting the environment variable AL_OPTS="-I/other/macro/dir"
before running ./autogen.sh .

Make sure pkg-config is installed.
])])

PKG_PROG_PKG_CONFIG

AC_MSG_NOTICE([checking the configure options...])

AC_DEFUN([KBD_SUBST_DEFINE], [
	AC_SUBST([$1], [$2])
	AC_DEFINE([$1], [$2], [$3])
])

KBD_SUBST_DEFINE([OLDKEYMAPDIR],  ["keytables"],    [Subdirectory of DATADIR])
KBD_SUBST_DEFINE([KEYMAPDIR],     ["keymaps"],      [Subdirectory of DATADIR])
KBD_SUBST_DEFINE([UNIMAPDIR],     ["unimaps"],      [Subdirectory of DATADIR])
KBD_SUBST_DEFINE([TRANSDIR],      ["consoletrans"], [Subdirectory of DATADIR])
KBD_SUBST_DEFINE([VIDEOMODEDIR],  ["videomodes"],   [Subdirectory of DATADIR])
KBD_SUBST_DEFINE([FONTDIR],       ["consolefonts"], [Subdirectory of DATADIR])
KBD_SUBST_DEFINE([PARTIALDIR],    ["partialfonts"], [Subdirectory of DATADIR])

AC_ARG_ENABLE([optional-progs],
	[AS_HELP_STRING([--enable-optional-progs],
			[Build and install a optional programs])],
	[OPTIONAL_PROGS=$enableval],
	[OPTIONAL_PROGS=no]
)
AM_CONDITIONAL(OPTIONAL_PROGS, test "$OPTIONAL_PROGS" = "yes")

AC_ARG_ENABLE([libkeymap],
	[AS_HELP_STRING([--enable-libkeymap],
			[Build and install a libkeymap])],
	[BUILD_LIBKEYMAP=$enableval],
	[BUILD_LIBKEYMAP=no]
)
AM_CONDITIONAL(BUILD_LIBKEYMAP, test "$BUILD_LIBKEYMAP" = "yes")

AC_ARG_ENABLE([libkfont],
	[AS_HELP_STRING([--enable-libkfont],
			[Build and install a libkfont])],
	[BUILD_LIBKFONT=$enableval],
	[BUILD_LIBKFONT=no]
)
AM_CONDITIONAL(BUILD_LIBKFONT, test "$BUILD_LIBKFONT" = "yes")

AS_IF([test "$BUILD_LIBKFONT" = "yes" -o "$BUILD_LIBKEYMAP" = "yes"],
	[BUILD_LIBKBDFILE=yes],
	[BUILD_LIBKBDFILE=no]
)
AM_CONDITIONAL(BUILD_LIBKBDFILE, test "$BUILD_LIBKBDFILE" = "yes")

AC_ARG_ENABLE([compress],
	[AS_HELP_STRING([--disable-compress],
			[Do not compress fonts and keymaps when they are installed.])],
	[enable_compress=$enableval],
	[enable_compress=auto]
)
AS_IF(
	[test "x$enable_compress" = xno],   [],
	[test "x$enable_compress" = xyes],  [AC_CHECK_PROGS([COMPRESS_PROG], [zstd xz bzip2 "gzip -n"])],
	[test "x$enable_compress" = xauto], [AC_CHECK_PROGS([COMPRESS_PROG], ["gzip -n"])],
	[AC_CHECK_PROGS([COMPRESS_PROG], ["$enable_compress"])]
)
AM_CONDITIONAL(ENABLE_COMPRESS, test "x$COMPRESS_PROG" != "x")

AC_ARG_ENABLE([vlock],
	[AS_HELP_STRING([--disable-vlock],
			[do not build vlock])],
	[VLOCK_PROG=$enableval],
	[VLOCK_PROG=yes]
)
AS_IF([test "$VLOCK_PROG" = "yes"], [
	PKG_CHECK_MODULES(PAM,      pam,      [HAVE_PAM=yes],      [HAVE_PAM=no])
	PKG_CHECK_MODULES(PAM_MISC, pam_misc, [HAVE_PAM_MISC=yes], [HAVE_PAM_MISC=no])
	AS_IF([test "$HAVE_PAM" != yes -o "$HAVE_PAM_MISC" != yes], [
		AC_CHECK_LIB(pam, pam_start, [
			AC_CHECK_HEADER([security/pam_appl.h],
				[HAVE_PAM=yes],
				AC_MSG_ERROR([Can't find required header files.]))
		])
		AC_CHECK_LIB(pam_misc, misc_conv, [
			AC_CHECK_HEADER([security/pam_misc.h],
				[HAVE_PAM_MISC=yes],
				AC_MSG_ERROR([Can't find required header files.]))
		])
		AS_IF([test "$HAVE_PAM" != yes -o "$HAVE_PAM_MISC" != yes], [
			AC_MSG_ERROR([libpam-devel required.])
		])
		AC_SUBST(PAM_LIBS, "-lpam -lpam_misc")
	])
])
AM_CONDITIONAL(VLOCK, test "$VLOCK_PROG" = "yes")

AC_ARG_WITH([zlib],
	[AS_HELP_STRING([--with-zlib],
			[support zlib compression @<:@default=auto@:>@])],
	[],
	[: m4_divert_text([DEFAULTS], [with_zlib=yes])]
)
AS_IF([test "$with_zlib" != "no"],
	[PKG_CHECK_MODULES(ZLIB, zlib, [HAVE_ZLIB=yes], [HAVE_ZLIB=no])],
	[HAVE_ZLIB=no]
)
AS_IF([test "$HAVE_ZLIB" = "yes"], [
	AC_DEFINE([HAVE_ZLIB], [1], [support zlib compression])
])
AM_CONDITIONAL(USE_ZLIB, test "$HAVE_ZLIB" = "yes")

AC_ARG_WITH([bzip2],
	[AS_HELP_STRING([--with-bzip2],
			[support bzip2 compression @<:@default=auto@:>@])],
	[],
	[: m4_divert_text([DEFAULTS], [with_bzip2=yes])]
)
AS_IF([test "$with_bzip2" != "no"],
	[PKG_CHECK_MODULES(BZIP2, bzip2, [HAVE_BZIP2=yes], [HAVE_BZIP2=no])],
	[HAVE_BZIP2=no]
)
AS_IF([test "$HAVE_BZIP2" = "no"], [
	AC_CHECK_LIB(bz2, BZ2_bzDecompressInit, [
		HAVE_BZIP2=yes
		BZIP2_LIBS=-lbz2
		BZIP2_CFLAGS=''
	], [HAVE_BZIP2=no])
])
AS_IF([test "$HAVE_BZIP2" = "yes"], [
	AC_DEFINE([HAVE_BZIP2], [1], [support bzip2 compression])
])
AM_CONDITIONAL(USE_BZIP2, test "$HAVE_BZIP2" = "yes")

AC_ARG_WITH([lzma],
	[AS_HELP_STRING([--with-lzma],
			[support lzma compression @<:@default=auto@:>@])],
	[],
	[: m4_divert_text([DEFAULTS], [with_lzma=yes])]
)
AS_IF([test "$with_lzma" != "no"],
	[PKG_CHECK_MODULES(LZMA, liblzma, [HAVE_LZMA=yes], [HAVE_LZMA=no])],
	[HAVE_LZMA=no]
)
AS_IF([test "$HAVE_LZMA" = "yes"], [
	AC_DEFINE([HAVE_LZMA], [1], [support lzma compression])
])
AM_CONDITIONAL(USE_LZMA, test "$HAVE_LZMA" = "yes")

AC_ARG_WITH([zstd],
	[AS_HELP_STRING([--with-zstd],
			[support zstd compression @<:@default=auto@:>@])],
	[],
	[: m4_divert_text([DEFAULTS], [with_zstd=yes])]
)
AS_IF([test "$with_zstd" != "no"],
	[PKG_CHECK_MODULES(ZSTD, libzstd, [HAVE_ZSTD=yes], [HAVE_ZSTD=no])],
	[HAVE_ZSTD=no]
)
AS_IF([test "$HAVE_ZSTD" = "yes"], [
	AC_DEFINE([HAVE_ZSTD], [1], [support zstd compression])
])
AM_CONDITIONAL(USE_ZSTD, test "$HAVE_ZSTD" = "yes")

AC_ARG_ENABLE([tests],
	[AS_HELP_STRING([--disable-tests],
			[do not build tests])],
	[build_tests=$enableval],
	[build_tests=auto]
)
AM_CONDITIONAL(BUILD_TESTS, test "$build_tests" != "no")

AC_ARG_ENABLE([memcheck],
	[AS_HELP_STRING([--disable-memcheck],
			[do not run memory checks (e.g. leak detection) during tests])],
	[tests_memcheck=$enableval],
	[tests_memcheck=auto]
)
AS_IF([test "$tests_memcheck" != "no"], [
	AC_CHECK_PROGS([VALGRIND_PROG], [valgrind])
	AS_IF([test "$tests_memcheck" != "auto" -a "x$VALGRIND_PROG" = x], [
		AC_MSG_ERROR([valgrind required.])
	])
])
AM_CONDITIONAL(TESTS_MEMCHECK, test "x$VALGRIND_PROG" != "x")

AC_CHECK_PROG([HAVE_DOXYGEN], [doxygen], [yes], [no])
AM_CONDITIONAL(HAVE_DOXYGEN, test "$HAVE_DOXYGEN" = "yes")

AC_MSG_NOTICE([generation of Makefiles...])

# Remove -h (dereference) from am__tar
AC_CONFIG_COMMANDS_PRE([
	AS_CASE([$am__tar],
		[*' -chf '*], [AC_SUBST([am__tar], [`AS_ECHO(["$am__tar"]) | $SED 's/ -chf / -cf /'`])],
		[*' chf '*],  [AC_SUBST([am__tar], [`AS_ECHO(["$am__tar"]) | $SED 's/ chf / cf /'`])])
])

AC_CONFIG_FILES([
	Makefile
	data/Makefile
	docs/Makefile
	docs/man/Makefile
	docs/man/man1/Makefile
	docs/man/man1misc/Makefile
	docs/man/man5/Makefile
	docs/man/man8/Makefile
	docs/man/man8misc/Makefile
	docs/reference/Makefile
	docs/reference/libkeymap/Makefile
	po/Makefile.in
	src/Makefile
	src/libcommon/Makefile
	src/libkbdfile/Makefile
	src/libkeymap/Makefile
	src/libkfont/Makefile
	src/vlock/Makefile
	tests/Makefile
	tests/helpers/Makefile
	tests/libcommon/Makefile
	tests/libkbdfile/Makefile
	tests/libkeymap/Makefile
])
AC_OUTPUT

AC_MSG_RESULT([
	$PACKAGE $VERSION

	prefix:                 ${prefix}
	libdir:                 ${libdir}
	bindir:                 ${bindir}
	datadir:                ${datadir}

	compiler:               ${CC}
	cflags:                 ${CFLAGS}
	code coverage:          ${enable_code_coverage}

	compress data:          ${COMPRESS_PROG}

	{get,set}keycodes:      ${KEYCODES_PROGS}
	resizecons:             ${RESIZECONS_PROGS}
	optional progs:         ${OPTIONAL_PROGS}
	vlock:                  ${VLOCK_PROG}
	standalone libkbdfile:  ${BUILD_LIBKBDFILE}
	- with dlopen zlib:     ${HAVE_ZLIB}
	- with dlopen libbz2:   ${HAVE_BZIP2}
	- with dlopen liblzma:  ${HAVE_LZMA}
	- with dlopen libzstd:  ${HAVE_ZSTD}
	standalone libkeymap:   ${BUILD_LIBKEYMAP}
	standalone libkfont:    ${BUILD_LIBKFONT}
])
