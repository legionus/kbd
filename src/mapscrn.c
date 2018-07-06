/*
 * mapscrn.c
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/kd.h>

#include <kbdfile.h>

#include "libcommon.h"

#include "paths.h"
#include "kdmapop.h"
#include "utf8.h"
#include "mapscrn.h"

/* search for the map file in these directories (with trailing /) */
static const char *const mapdirpath[]  = { "", DATADIR "/" TRANSDIR "/", 0 };
static const char *const mapsuffixes[] = { "", ".trans", "_to_uni.trans", ".acm", 0 };

int verbose = 0;
int debug   = 0;

int main(int argc, char *argv[])
{
	int fd;

	set_progname(argv[0]);

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE_NAME, LOCALEDIR);
	textdomain(PACKAGE_NAME);

	if (argc == 2 && !strcmp(argv[1], "-V"))
		print_version_and_exit();

	if (argc > 1 && !strcmp(argv[1], "-v")) {
		verbose = 1;
		argc--;
		argv++;
	}

	if ((fd = getfd(NULL)) < 0)
		kbd_error(EXIT_FAILURE, 0, _("Couldn't get a file descriptor referring to the console"));

	if (argc >= 3 && !strcmp(argv[1], "-o")) {
		if (saveoldmap(fd, argv[2]) < 0)
			exit(EXIT_FAILURE);
		argc -= 2;
		argv += 2;
		if (argc == 1)
			exit(EXIT_SUCCESS);
	}

	if (argc != 2) {
		fprintf(stderr, _("usage: %s [-V] [-v] [-o map.orig] map-file\n"),
		        get_progname());
		exit(EXIT_FAILURE);
	}
	loadnewmap(fd, argv[1]);
	exit(EXIT_SUCCESS);
}
