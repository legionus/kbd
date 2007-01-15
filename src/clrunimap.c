/*
 * clrunimap.c
 *
 * Note: nowadays this kills kernel console output!
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/kd.h>
#include "kdmapop.h"
#include "getfd.h"
#include "nls.h"

int
main(int argc, char *argv[]) {
	int fd;

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	fd = getfd();

	return loadunimap (fd, NULL, NULL);
}
