/*
 * disalloc.c - aeb - 940501 - Disallocate virtual terminal(s)
 * Renamed deallocvt.
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/vt.h>
#include "getfd.h"
#include "nls.h"
#include "version.h"
#include "kbd_error.h"

int main(int argc, char *argv[])
{
	int fd, num, i;

	if (argc < 1) /* unlikely */
		return EXIT_FAILURE;
	set_progname(argv[0]);

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE_NAME, LOCALEDIR);
	textdomain(PACKAGE_NAME);

	if (argc == 2 && !strcmp(argv[1], "-V"))
		print_version_and_exit();

	for (i = 1; i < argc; i++) {
		if (!isdigit(argv[i][0])) {
			fprintf(stderr, _("%s: unknown option\n"), progname);
			return EXIT_FAILURE;
		}
	}

	if ((fd = getfd(NULL)) < 0)
		kbd_error(EXIT_FAILURE, 0, _("Couldn't get a file descriptor referring to the console"));

	if (argc == 1) {
		/* deallocate all unused consoles */
		if (ioctl(fd, VT_DISALLOCATE, 0)) {
			kbd_error(EXIT_FAILURE, errno, "ioctl VT_DISALLOCATE");
		}
	} else
		for (i = 1; i < argc; i++) {
			num = atoi(argv[i]);
			if (num == 0) {
				kbd_error(EXIT_FAILURE, 0, _("0: illegal VT number\n"));
			} else if (num == 1) {
				kbd_error(EXIT_FAILURE, 0, _("VT 1 is the console and cannot be deallocated\n"));
			} else if (ioctl(fd, VT_DISALLOCATE, num)) {
				kbd_error(EXIT_FAILURE, errno, _("could not deallocate console %d: "
				                                 "ioctl VT_DISALLOCATE"),
				          num);
			}
		}
	exit(EXIT_SUCCESS);
}
