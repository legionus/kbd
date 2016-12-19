/*
 * fgconsole.c - aeb - 960123 - Print foreground console
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <getopt.h>
#include <linux/vt.h>
#include <linux/serial.h>
#include "getfd.h"
#include "nls.h"
#include "version.h"
#include "kbd_error.h"

static void __attribute__((noreturn))
usage(void)
{
	fprintf(stderr, _("%s version %s\n"
	                  "\n"
	                  "Usage: %s [options]\n"
	                  "\n"
	                  "Valid options are:\n"
	                  "\n"
	                  "	-h --help            display this help text\n"
	                  "	-V --version         display program version\n"
	                  "	-n --next-available  display number of next unallocated VT\n"),
	        progname, PACKAGE_VERSION, progname);
	exit(EXIT_FAILURE);
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

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE_NAME, LOCALEDIR);
	textdomain(PACKAGE_NAME);

	set_progname(argv[0]);
	while ((c = getopt_long(argc, argv, "Vhn", long_opts, NULL)) != EOF) {
		switch (c) {
			case 'h':
				usage();
				exit(0);
			case 'n':
				show_vt = 1;
				break;
			case 'V':
				print_version_and_exit();
				break;
			case '?':
				usage();
				exit(1);
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
