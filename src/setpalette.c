#include "config.h"

#include <linux/kd.h>
#include <sys/ioctl.h>

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <sysexits.h>
#include <errno.h>

#include "libcommon.h"

static void KBD_ATTR_NORETURN
usage(int rc, const struct kbd_help *options)
{
	fprintf(stderr, "Usage: %s [option...] <index> <red> <green> <blue>\n",
			program_invocation_short_name);

	print_options(options);
	print_report_bugs();

	exit(rc);
}

int main(int argc, char **argv)
{
	int c, fd, indx, red, green, blue;
	unsigned char cmap[48];
	char *console = NULL;

	const char *short_opts = "C:hV";
	const struct option long_opts[] = {
		{ "console", required_argument, NULL, 'C' },
		{ "help",    no_argument,       NULL, 'h' },
		{ "version", no_argument,       NULL, 'V' },
		{ NULL,      0,                 NULL,  0  }
	};
	const struct kbd_help opthelp[] = {
		{ "-C, --console=DEV", _("the console device to be used.") },
		{ "-V, --version",     _("print version number.")     },
		{ "-h, --help",        _("print this usage message.") },
		{ NULL, NULL }
	};

	setuplocale();

	while ((c = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
		switch (c) {
			case 'C':
				if (optarg == NULL || optarg[0] == '\0')
					usage(EX_USAGE, opthelp);
				console = optarg;
				break;
			case 'V':
				print_version_and_exit();
				break;
			case 'h':
				usage(EXIT_SUCCESS, opthelp);
				break;
			case '?':
				usage(EX_USAGE, opthelp);
				break;
		}
	}

	if ((argc - optind) != 4)
		usage(EX_USAGE, opthelp);

	indx  = atoi(argv[optind++]);
	red   = atoi(argv[optind++]);
	green = atoi(argv[optind++]);
	blue  = atoi(argv[optind++]);

	if (indx < 0 || red < 0 || green < 0 || blue < 0 ||
	    indx > 15 || red > 255 || green > 255 || blue > 255)
		kbd_error(EXIT_FAILURE, 0, "indx must be in 0..15, colors in 0..255");

	if ((fd = getfd(console)) < 0)
		kbd_error(EXIT_FAILURE, 0, _("Couldn't get a file descriptor referring to the console."));

	if (ioctl(fd, GIO_CMAP, cmap))
		kbd_error(EXIT_FAILURE, errno, "ioctl GIO_CMAP");

	cmap[3 * indx]     = (unsigned char) red;
	cmap[3 * indx + 1] = (unsigned char) green;
	cmap[3 * indx + 2] = (unsigned char) blue;

	if (ioctl(fd, PIO_CMAP, cmap))
		kbd_error(EXIT_FAILURE, errno, "ioctl PIO_CMAP");

	return EXIT_SUCCESS;
}
