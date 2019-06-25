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
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sysexits.h>
#include <sys/ioctl.h>
#include <linux/kd.h>

#include "libcommon.h"

static void __attribute__((noreturn))
usage(void)
{
	fprintf(stderr, _("usage: kbd_mode [-a|-u|-k|-s] [-f] [-C device]\n"));
	exit(EX_USAGE);
}

static void
fprint_mode(FILE *stream, int  mode)
{
	switch (mode) {
		case K_RAW:
			fprintf(stream, _("The keyboard is in raw (scancode) mode\n"));
			break;
		case K_MEDIUMRAW:
			fprintf(stream, _("The keyboard is in mediumraw (keycode) mode\n"));
			break;
		case K_XLATE:
			fprintf(stream, _("The keyboard is in the default (ASCII) mode\n"));
			break;
		case K_UNICODE:
			fprintf(stream, _("The keyboard is in Unicode (UTF-8) mode\n"));
			break;
		default:
			fprintf(stream, _("The keyboard is in some unknown mode\n"));
        }
}

int main(int argc, char *argv[])
{
	int fd, mode, orig_mode, c, n = 0, force = 0;
	char *console = NULL;

	set_progname(argv[0]);
	setuplocale();

	if (argc == 2 && !strcmp(argv[1], "-V"))
		print_version_and_exit();

	while ((c = getopt(argc, argv, "auskfC:")) != EOF) {
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
			case 'f':
				force = 1;
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
		fprint_mode(stdout, mode);
		return EXIT_SUCCESS;
	}

	if (force == 0) {
		/* only perform safe mode switches */
		if (ioctl(fd, KDGKBMODE, &orig_mode)) {
			kbd_error(EXIT_FAILURE, errno, "ioctl KDGKBMODE");
		}

		if (mode == orig_mode) {
			/* skip mode change */
			return EXIT_SUCCESS;
		}

		if ((mode == K_XLATE && orig_mode != K_UNICODE) || (mode == K_UNICODE && orig_mode != K_XLATE)) {
			fprint_mode(stderr, orig_mode);
			fprintf(stderr, _("Changing to the requested mode may make "
				"your keyboard unusable, please use -f to force the change.\n"));
			return EXIT_FAILURE;
		}
	}
	if (ioctl(fd, KDSKBMODE, mode)) {
		kbd_error(EXIT_FAILURE, errno, "ioctl KDSKBMODE");
	}

	return EXIT_SUCCESS;
}
