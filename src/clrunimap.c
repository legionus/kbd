/*
 * clrunimap.c
 *
 * Note: nowadays this kills kernel console output!
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/kd.h>
#include <sys/ioctl.h>
#include "getfd.h"
#include "nls.h"

int
main(int argc, char *argv[]) {
	struct unimapinit advice;
	int fd;

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	fd = getfd();

	advice.advised_hashsize = 0;
	advice.advised_hashstep = 0;
	advice.advised_hashlevel = 0;

	if(ioctl(fd, PIO_UNIMAPCLR, &advice)) {
		perror("PIO_UNIMAPCLR");
		exit(1);
	}

	return 0;
}
