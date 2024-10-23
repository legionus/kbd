/*
 * setlogcons.c - aeb - 000523
 *
 * usage: setlogcons N
 */

/* Send kernel messages to the current console or to console N */
#include "config.h"

#include <linux/tiocl.h>
#include <sys/ioctl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <limits.h>
#include <sysexits.h>
#include <getopt.h>

#include "libcommon.h"

static void KBD_ATTR_NORETURN
usage(int rc, const struct kbd_help *options)
{
	fprintf(stderr, "Usage: %s [option...] [N]\n", program_invocation_short_name);
	fprintf(stderr, "\n");
	fprintf(stderr, "Send kernel messages to the current console or to console N\n");

	print_options(options);
	print_report_bugs();

	exit(rc);
}

int main(int argc, char **argv)
{
	int c, fd, cons;
	struct {
		char fn, subarg;
	} arg;
	char *console = NULL;

	setuplocale();

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

	cons = 0; /* current console */

	if (optind < argc) {
		cons = atoi(argv[optind]);

		if (cons < 0)
			kbd_error(EX_DATAERR, 0,
				  "Argument must be positive: %s", argv[optind]);

		if (cons > CHAR_MAX)
			kbd_error(EX_DATAERR, 0,
				  "Argument is too big: %s", argv[optind]);
	}

	if ((fd = getfd(console)) < 0)
		kbd_error(EX_OSERR, 0, _("Couldn't get a file descriptor referring to the console."));

	arg.fn     = TIOCL_SETKMSGREDIRECT; /* redirect kernel messages */
	arg.subarg = (char) cons;           /* to specified console */

	if (ioctl(fd, TIOCLINUX, &arg))
		kbd_error(EX_OSERR, errno, "TIOCLINUX");

	return EX_OK;
}
