/*
 * setlogcons.c - aeb - 000523
 *
 * usage: setlogcons N
 */

/* Send kernel messages to the current console or to console N */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#include "libcommon.h"

int main(int argc, char **argv)
{
	int fd, cons;
	struct {
		char fn, subarg;
	} arg;

	set_progname(argv[0]);
	setuplocale();

	if (argc == 2)
		cons = atoi(argv[1]);
	else
		cons = 0; /* current console */

	if ((fd = getfd(NULL)) < 0)
		kbd_error(EXIT_FAILURE, 0, _("Couldn't get a file descriptor referring to the console"));

	arg.fn     = 11;   /* redirect kernel messages */
	arg.subarg = cons; /* to specified console */
	if (ioctl(fd, TIOCLINUX, &arg)) {
		kbd_error(EXIT_FAILURE, errno, "TIOCLINUX");
	}
	return EXIT_SUCCESS;
}
