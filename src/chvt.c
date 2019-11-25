/*
 * chvt.c - aeb - 940227 - Change virtual terminal
 */
#include "config.h"

#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/vt.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <sysexits.h>

#include "libcommon.h"

static void __attribute__((noreturn))
usage(int rc)
{
	fprintf(stderr, _("Usage: %s [option...] N\n"
	                  "\n"
	                  "Options:\n"
	                  "  -t, --timeout=NUM     set deadline to NUM seconds to change virtual terminal;\n"
	                  "  -h, --help            print this usage message;\n"
	                  "  -V, --version         print version number.\n"),
		get_progname());
	exit(rc);
}

static void __attribute__((noreturn))
sigalrmhandler(int sig __attribute__((unused)))
{
	kbd_error(EXIT_FAILURE, 0, _("Failed to change virtual terminal"));
}

int main(int argc, char *argv[])
{
	int c, fd, num;
	int secs = 0;

	const char *const short_opts = "t:hV";
	const struct option long_opts[] = {
		{ "help",    no_argument, NULL, 'h' },
		{ "version", no_argument, NULL, 'V' },
		{ "timeout", required_argument, NULL, 't' },
		{ NULL, 0, NULL, 0 }
	};

	set_progname(argv[0]);
	setuplocale();

	while ((c = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
		switch (c) {
			case 't':
				secs = atoi(optarg);
				break;
			case 'V':
				print_version_and_exit();
				break;
			case 'h':
				usage(EXIT_SUCCESS);
				break;
			case '?':
				usage(EX_USAGE);
				break;
		}
	}

	if (argc == optind) {
		fprintf(stderr, _("Argument required\n"));
		usage(EX_USAGE);
	}

	if ((fd = getfd(NULL)) < 0)
		kbd_error(EXIT_FAILURE, 0, _("Couldn't get a file descriptor referring to the console"));

	num = atoi(argv[optind]);

	if (secs > 0) {
		signal(SIGALRM, sigalrmhandler);
		alarm((unsigned int) secs);
	}

	if (ioctl(fd, VT_ACTIVATE, num)) {
		kbd_error(EXIT_FAILURE, errno, "ioctl VT_ACTIVATE");
	}

	if (ioctl(fd, VT_WAITACTIVE, num)) {
		kbd_error(EXIT_FAILURE, errno, "ioctl VT_WAITACTIVE");
	}

	return EXIT_SUCCESS;
}
