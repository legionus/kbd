/*
 * totextmode.c - aeb - 2000-01-20
 */
#include "config.h"

#include <stdlib.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/vt.h>
#include <linux/kd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include "libcommon.h"

int main(int argc, char *argv[])
{
	int fd;

	setuplocale();

	if (argc == 2 && !strcmp(argv[1], "-V"))
		print_version_and_exit();

	if (argc != 2) {
		kbd_error(EXIT_FAILURE, 0, _("Usage: %s [option...]\n"), program_invocation_short_name);
	}

	if ((fd = getfd(NULL)) < 0)
		kbd_error(EXIT_FAILURE, 0, _("Couldn't get a file descriptor referring to the console."));

	if (ioctl(fd, KDSETMODE, KD_TEXT)) {
		kbd_error(EXIT_FAILURE, errno, "totextmode: KDSETMODE");
	}
	return EXIT_SUCCESS;
}
