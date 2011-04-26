/*
 * setvesablank.c - aeb - 941230
 *
 * usage: setvesablank ON|on|off
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include "getfd.h"
#include "nls.h"

int
main(int argc, char *argv[]) {
	int fd;
	struct { char ten, onoff; } arg;

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE_NAME, LOCALEDIR);
	textdomain(PACKAGE_NAME);

	if (argc != 2) {
		fprintf(stderr, _("usage: %s\n"), "setvesablank ON|on|off");
		exit(1);
	}
	fd = getfd(NULL);
	arg.ten = 10;
	arg.onoff = 0;
	if (!strcmp(argv[1], "on"))
		arg.onoff = 1;
	else if (!strcmp(argv[1], "ON"))
		arg.onoff = 2;
	if (ioctl(fd, TIOCLINUX, &arg)) {
		perror("setvesablank: TIOCLINUX");
		exit(1);
	}
	return 0;
}
