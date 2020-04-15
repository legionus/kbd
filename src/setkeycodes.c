/*
 * call: setkeycode scancode keycode ...
 *  (where scancode is either xx or e0xx, given in hexadecimal,
 *   and keycode is given in decimal)
 *
 * aeb, 941108, 2004-01-11
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <sysexits.h>
#include <sys/ioctl.h>
#include <linux/kd.h>

#include "libcommon.h"

static void __attribute__((noreturn))
usage(const char *s)
{
	fprintf(stderr, "setkeycode: %s\n", s);
	fprintf(stderr, _(
	                    "usage: setkeycode scancode keycode ...\n"
	                    " (where scancode is either xx or e0xx, given in hexadecimal,\n"
	                    "  and keycode is given in decimal)\n"));
	exit(EX_USAGE);
}

static void
str_to_uint(const char *str, int base, unsigned int *res)
{
	char *ep;
	long int v;

	errno = 0;
	v = strtol(str, &ep, base);

	if (*ep)
		usage(_("error reading scancode"));

	if (errno == ERANGE)
		kbd_error(EX_DATAERR, 0, "Argument out of range: %s", str);

	if (v < 0)
		kbd_error(EX_DATAERR, 0, "Argument must be positive: %s", str);

	if (v > UINT_MAX)
		kbd_error(EX_DATAERR, 0, "Argument is too big: %s", str);

	*res = (unsigned int) v;
}

int main(int argc, char **argv)
{
	int fd;
	struct kbkeycode a;

	set_progname(argv[0]);
	setuplocale();

	if (argc == 2 && !strcmp(argv[1], "-V"))
		print_version_and_exit();

	if (argc % 2 != 1)
		usage(_("even number of arguments expected"));

	if ((fd = getfd(NULL)) < 0)
		kbd_error(EX_OSERR, 0, _("Couldn't get a file descriptor referring to the console"));

	while (argc > 2) {
		str_to_uint(argv[1], 16, &a.scancode);
		str_to_uint(argv[2], 0, &a.keycode);

		if (a.scancode >= 0xe000) {
			a.scancode -= 0xe000;
			a.scancode += 128; /* some kernels needed +256 */
		}
#if 0
		/* Test is OK up to 2.5.31--later kernels have more keycodes */
		if (a.scancode > 255 || a.keycode > 127)
			usage(_("code outside bounds"));

		/* Both fields are unsigned int, so can be large;
		   for current kernels the correct test might be
		     (a.scancode > 255 || a.keycode > 239)
		   but we can leave testing to the kernel. */
#endif
		if (ioctl(fd, KDSETKEYCODE, &a)) {
			kbd_error(EXIT_FAILURE, errno,
			          _("failed to set scancode %x to keycode %d: ioctl KDSETKEYCODE"),
			          a.scancode, a.keycode);
		}
		argc -= 2;
		argv += 2;
	}
	return EX_OK;
}
