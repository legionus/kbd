/*
 * disalloc.c - aeb - 940501 - Disallocate virtual terminal(s)
 * Renamed deallocvt.
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/vt.h>
#include <getopt.h>
#include <sysexits.h>

#include "libcommon.h"

static void KBD_ATTR_NORETURN
usage(int rc, const struct kbd_help *options)
{
	fprintf(stderr, _("Usage: %s [option...] [N ...]\n"), program_invocation_short_name);

	print_options(options);
	print_report_bugs();

	exit(rc);
}

int main(int argc, char *argv[])
{
	int fd, num, i;
	const char *console = NULL;

	if (argc < 1) /* unlikely */
		return EXIT_FAILURE;

	const char *const short_opts = "C:hV";
	const struct option long_opts[] = {
		{ "console", required_argument, NULL, 'C' },
		{ "help",    no_argument,       NULL, 'h' },
		{ "version", no_argument,       NULL, 'V' },
		{ NULL, 0, NULL, 0 }
	};

	const struct kbd_help opthelp[] = {
		{ "-C, --console=DEV", _("the console device to be used.") },
		{ "-h, --help",        _("print this usage message.")      },
		{ "-V, --version",     _("print version number.")          },
		{ NULL, NULL }
	};

	setuplocale();

	while ((i = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
		switch (i) {
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
			case '?':
				usage(EX_USAGE, opthelp);
		}
	}

	for (i = optind; i < argc; i++) {
		if (!isdigit(argv[i][0]))
			kbd_error(EX_USAGE, 0, _("Unrecognized argument: %s"), argv[i]);
	}

	if ((fd = getfd(console)) < 0)
		kbd_error(EXIT_FAILURE, 0, _("Couldn't get a file descriptor referring to the console."));

	if (optind == argc) {
		/* deallocate all unused consoles */
		if (ioctl(fd, VT_DISALLOCATE, 0)) {
			kbd_error(EXIT_FAILURE, errno, "ioctl VT_DISALLOCATE");
		}
	} else
		for (i = optind; i < argc; i++) {
			num = atoi(argv[i]);
			if (num == 0) {
				kbd_error(EXIT_FAILURE, 0, _("0: illegal VT number"));
			} else if (num == 1) {
				kbd_error(EXIT_FAILURE, 0, _("VT 1 is the console and cannot be deallocated"));
			} else if (ioctl(fd, VT_DISALLOCATE, num)) {
				kbd_error(EXIT_FAILURE, errno, _("could not deallocate console %d: "
				                                 "ioctl VT_DISALLOCATE"),
				          num);
			}
		}
	exit(EXIT_SUCCESS);
}
