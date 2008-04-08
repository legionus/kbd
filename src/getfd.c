#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/kd.h>
#include "nls.h"
#include "getfd.h"

/*
 * getfd.c
 *
 * Get an fd for use with kbd/console ioctls.
 * We try several things because opening /dev/console will fail
 * if someone else used X (which does a chown on /dev/console).
 */

static int
is_a_console(int fd) {
	char arg;

	arg = 0;
	return (isatty (fd)
		&& ioctl(fd, KDGKBTYPE, &arg) == 0
		&& ((arg == KB_101) || (arg == KB_84)));
}

static int
open_a_console(const char *fnam) {
	int fd;

	/*
	 * For ioctl purposes we only need some fd and permissions
	 * do not matter. But setfont:activatemap() does a write.
	 */
	fd = open(fnam, O_RDWR);
	if (fd < 0)
		fd = open(fnam, O_WRONLY);
	if (fd < 0)
		fd = open(fnam, O_RDONLY);
	if (fd < 0)
		return -1;
	if (!is_a_console(fd)) {
		close(fd);
		return -1;
	}
	return fd;
}

int getfd(const char *fnam) {
	int fd;

	if (fnam) {
		fd = open_a_console(fnam);
		if (fd >= 0)
			return fd;
		fprintf(stderr,
			_("Couldn't open %s\n"), fnam);
		exit(1);
	}

	fd = open_a_console("/proc/self/fd/0");
	if (fd >= 0)
		return fd;

	fd = open_a_console("/dev/tty");
	if (fd >= 0)
		return fd;

	fd = open_a_console("/dev/tty0");
	if (fd >= 0)
		return fd;

	fd = open_a_console("/dev/vc/0");
	if (fd >= 0)
		return fd;

	fd = open_a_console("/dev/console");
	if (fd >= 0)
		return fd;

	for (fd = 0; fd < 3; fd++)
		if (is_a_console(fd))
			return fd;

	fprintf(stderr,
		_("Couldn't get a file descriptor referring to the console\n"));
	exit(1);		/* total failure */
}
