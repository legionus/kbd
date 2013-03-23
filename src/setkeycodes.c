/*
 * call: setkeycode scancode keycode ...
 *  (where scancode is either xx or e0xx, given in hexadecimal,
 *   and keycode is given in decimal)
 *
 * aeb, 941108, 2004-01-11
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/kd.h>
#include "getfd.h"
#include "nls.h"
#include "version.h"

static void __attribute__ ((noreturn))
usage(char *s) {
	fprintf(stderr, "setkeycode: %s\n", s);
	fprintf(stderr, _(
	    "usage: setkeycode scancode keycode ...\n"
	    " (where scancode is either xx or e0xx, given in hexadecimal,\n"
	    "  and keycode is given in decimal)\n"));
	exit(1);
}

int
main(int argc, char **argv) {
	char *ep;
	int fd;
	struct kbkeycode a;

	set_progname(argv[0]);

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE_NAME, LOCALEDIR);
	textdomain(PACKAGE_NAME);

	if (argc == 2 && !strcmp(argv[1], "-V"))
		print_version_and_exit();

	if (argc % 2 != 1)
		usage(_("even number of arguments expected"));
	fd = getfd(NULL);

	while (argc > 2) {
		a.keycode = atoi(argv[2]);
		a.scancode = strtol(argv[1], &ep, 16);
		if (*ep)
			usage(_("error reading scancode"));
		if (a.scancode >= 0xe000) {
			a.scancode -= 0xe000;
			a.scancode += 128;	/* some kernels needed +256 */
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
		if (ioctl(fd,KDSETKEYCODE,&a)) {
			perror("KDSETKEYCODE");
			fprintf(stderr,
				_("failed to set scancode %x to keycode %d\n"),
				a.scancode, a.keycode);
			exit(1);
		}
		argc -= 2;
		argv += 2;
	}
	return 0;
}
