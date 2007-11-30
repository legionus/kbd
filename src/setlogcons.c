/*
 * setlogcons.c - aeb - 000523
 *
 * usage: setlogcons N
 */

/* Send kernel messages to the current console or to console N */
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include "getfd.h"
#include "nls.h"

int
main(int argc, char **argv){
	int fd, cons;
	struct { char fn, subarg; } arg;

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE_NAME, LOCALEDIR);
	textdomain(PACKAGE_NAME);

	if (argc == 2)
		cons = atoi(argv[1]);
	else
		cons = 0;	/* current console */

	fd = getfd(NULL);
	arg.fn = 11;		/* redirect kernel messages */
	arg.subarg = cons;	/* to specified console */
	if (ioctl(fd, TIOCLINUX, &arg)) {
		perror("TIOCLINUX");
		exit(1);
	}
	return 0;
}
