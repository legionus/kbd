/*
 * chvt.c - aeb - 940227 - Change virtual terminal
 */
#include "config.h"

#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/vt.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include "getfd.h"
#include "nls.h"
#include "version.h"
#include "kbd_error.h"

int main(int argc, char *argv[])
{
	int fd, num;

	set_progname(argv[0]);

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE_NAME, LOCALEDIR);
	textdomain(PACKAGE_NAME);

	if (argc == 2 && !strcmp(argv[1], "-V"))
		print_version_and_exit();

	if (argc != 2) {
		fprintf(stderr, _("usage: chvt N\n"));
		return EXIT_FAILURE;
	}

	if ((fd = getfd(NULL)) < 0)
		kbd_error(EXIT_FAILURE, 0, _("Couldn't get a file descriptor referring to the console"));

	num = atoi(argv[1]);

	if (ioctl(fd, VT_ACTIVATE, num)) {
		kbd_error(EXIT_FAILURE, errno, "ioctl VT_ACTIVATE");
	}

	if (ioctl(fd, VT_WAITACTIVE, num)) {
		kbd_error(EXIT_FAILURE, errno, "ioctl VT_WAITACTIVE");
	}

	return EXIT_SUCCESS;
}
