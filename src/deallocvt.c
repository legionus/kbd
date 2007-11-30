/*
 * disalloc.c - aeb - 940501 - Disallocate virtual terminal(s)
 * Renamed deallocvt.
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/vt.h>
#include "getfd.h"
#include "nls.h"
#include "version.h"

int
main(int argc, char *argv[]) {
	int fd, num, i;

	if (argc < 1)		/* unlikely */
		exit(1);
	set_progname(argv[0]);

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE_NAME, LOCALEDIR);
	textdomain(PACKAGE_NAME);

	if (argc == 2 && !strcmp(argv[1], "-V"))
		print_version_and_exit();

	for (i = 1; i < argc; i++) {
		if (!isdigit(argv[i][0])) {
			fprintf(stderr, _("%s: unknown option\n"), progname);
			exit(1);
		}
	}

	fd = getfd(NULL);

	if (argc == 1) {
		/* deallocate all unused consoles */
		if (ioctl(fd,VT_DISALLOCATE,0)) {
			perror("VT_DISALLOCATE");
			fprintf(stderr,
				_("%s: deallocating all unused consoles failed\n"),
				progname);
			exit(1);
		}
	} else for (i = 1; i < argc; i++) {
		num = atoi(argv[i]);
		if (num == 0) {
			fprintf(stderr,
				_("%s: 0: illegal VT number\n"), progname);
			exit(1);
		} else if (num == 1) {
			fprintf(stderr,
				_("%s: VT 1 is the console and cannot be deallocated\n"),
				progname);
			exit(1);
		} else if (ioctl(fd,VT_DISALLOCATE,num)) {
			perror("VT_DISALLOCATE");
			fprintf(stderr,
				_("%s: could not deallocate console %d\n"),
				progname, num);
			exit(1);
		}
	}
	exit(0);
}
