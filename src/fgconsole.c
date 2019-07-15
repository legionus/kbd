/*
 * fgconsole.c - aeb - 960123 - Print foreground console
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <getopt.h>
#include <sysexits.h>
#include <linux/vt.h>
#include <linux/serial.h>

#include "libcommon.h"

static void __attribute__((noreturn))
usage(int rc)
{
	const char *progname = get_progname();
	fprintf(stderr, _(
		"%s version %s\n"
		"\n"
		"Usage: %s [options]\n"
		"\n"
		"Options:\n"
		"\n"
		"  -n, --next-available  print number of next unallocated VT\n"
		"  -h, --help            print this usage message;\n"
		"  -V, --version         print version number.\n"),
	        progname, PACKAGE_VERSION, progname);
	exit(rc);
}

int main(int argc, char **argv)
{
	struct vt_stat vtstat;
	int fd, vtno = -1, c, show_vt = 0;
	struct serial_struct sr;
	const struct option long_opts[] = {
		{ "help", no_argument, NULL, 'h' },
		{ "version", no_argument, NULL, 'V' },
		{ "next-available", no_argument, NULL, 'n' },
		{ NULL, 0, NULL, 0 }
	};

	set_progname(argv[0]);
	setuplocale();

	while ((c = getopt_long(argc, argv, "Vhn", long_opts, NULL)) != EOF) {
		switch (c) {
			case 'n':
				show_vt = 1;
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

	if ((fd = getfd(NULL)) < 0)
		kbd_error(EXIT_FAILURE, 0, _("Couldn't get a file descriptor referring to the console"));

	if (show_vt) {
		if ((ioctl(fd, VT_OPENQRY, &vtno) < 0) || vtno == -1) {
			kbd_error(2, errno, _("Couldn't read VTNO: "));
		}
		printf("%d\n", vtno);
		return EXIT_SUCCESS;
	}

	if (ioctl(fd, TIOCGSERIAL, &sr) == 0) {
		printf("serial\n");
		return EXIT_SUCCESS;
	}

	if (ioctl(fd, VT_GETSTATE, &vtstat)) {
		kbd_error(EXIT_FAILURE, errno, "fgconsole: VT_GETSTATE");
	}
	printf("%d\n", vtstat.v_active);
	return EXIT_SUCCESS;
}
