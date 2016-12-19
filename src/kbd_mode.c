/*
 * kbd_mode: report and set keyboard mode - aeb, 940406
 * 
 * If you make \215A\201 an alias for "kbd_mode -a", and you are
 * in raw mode, then hitting F7 = (two keys) will return you to sanity.
 */
#include "config.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/kd.h>
#include "getfd.h"
#include "nls.h"
#include "version.h"
#include "kbd_error.h"

static void __attribute__((noreturn))
usage(void)
{
	fprintf(stderr, _("usage: kbd_mode [-a|-u|-k|-s] [-C device]\n"));
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
	int fd, mode, c, n = 0;
	char *console = NULL;

	set_progname(argv[0]);

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE_NAME, LOCALEDIR);
	textdomain(PACKAGE_NAME);

	if (argc == 2 && !strcmp(argv[1], "-V"))
		print_version_and_exit();

	while ((c = getopt(argc, argv, "auskC:")) != EOF) {
		switch (c) {
			case 'a':
				if (n > 0)
					usage();
				mode = K_XLATE;
				n++;
				break;
			case 'u':
				if (n > 0)
					usage();
				mode = K_UNICODE;
				n++;
				break;
			case 's':
				if (n > 0)
					usage();
				mode = K_RAW;
				n++;
				break;
			case 'k':
				if (n > 0)
					usage();
				mode = K_MEDIUMRAW;
				n++;
				break;
			case 'C':
				if (!optarg || !optarg[0])
					usage();
				console = optarg;
				break;
			default:
				usage();
		}
	}

	if ((fd = getfd(console)) < 0)
		kbd_error(EXIT_FAILURE, 0, _("Couldn't get a file descriptor referring to the console"));

	if (n == 0) {
		/* report mode */
		if (ioctl(fd, KDGKBMODE, &mode)) {
			kbd_error(EXIT_FAILURE, errno, "ioctl KDGKBMODE");
		}
		switch (mode) {
			case K_RAW:
				printf(_("The keyboard is in raw (scancode) mode\n"));
				break;
			case K_MEDIUMRAW:
				printf(_("The keyboard is in mediumraw (keycode) mode\n"));
				break;
			case K_XLATE:
				printf(_("The keyboard is in the default (ASCII) mode\n"));
				break;
			case K_UNICODE:
				printf(_("The keyboard is in Unicode (UTF-8) mode\n"));
				break;
			default:
				printf(_("The keyboard is in some unknown mode\n"));
		}
		return EXIT_SUCCESS;
	}

	if (ioctl(fd, KDSKBMODE, mode)) {
		kbd_error(EXIT_FAILURE, errno, "ioctl KDSKBMODE");
	}

	return EXIT_SUCCESS;
}
