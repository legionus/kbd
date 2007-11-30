/*
 * chvt.c - aeb - 940227 - Change virtual terminal
 */
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/vt.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "getfd.h"
#include "nls.h"
#include "version.h"

int
main(int argc, char *argv[]) {
    int fd, num;

    set_progname(argv[0]);

    setlocale(LC_ALL, "");
    bindtextdomain(PACKAGE_NAME, LOCALEDIR);
    textdomain(PACKAGE_NAME);

    if (argc == 2 && !strcmp(argv[1], "-V"))
	print_version_and_exit();

    if (argc != 2) {
	fprintf(stderr, _("usage: chvt N\n"));
	exit(1);
    }
    fd = getfd(NULL);
    num = atoi(argv[1]);
    if (ioctl(fd,VT_ACTIVATE,num)) {
	perror("chvt: VT_ACTIVATE");
	exit(1);
    }
    if (ioctl(fd,VT_WAITACTIVE,num)) {
	perror("VT_WAITACTIVE");
	exit(1);
    }
    exit(0);
}
