/*
 * mapscrn.c
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <fcntl.h>
#include <sysexits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/kd.h>

#include "libcommon.h"
#include "kfont.h"

int main(int argc, char *argv[])
{
	int fd, ret;
	struct kfont_context *kfont;

	set_progname(argv[0]);
	setuplocale();

	if ((ret = kfont_init(get_progname(), &kfont)) < 0)
		return -ret;

	if (argc == 2 && !strcmp(argv[1], "-V"))
		print_version_and_exit();

	if (argc > 1 && !strcmp(argv[1], "-v")) {
		kfont_inc_verbosity(kfont);
		argc--;
		argv++;
	}

	if ((fd = getfd(NULL)) < 0)
		kbd_error(EXIT_FAILURE, 0, _("Couldn't get a file descriptor referring to the console"));

	if (argc >= 3 && !strcmp(argv[1], "-o")) {
		if ((ret = kfont_save_consolemap(kfont, fd, argv[2])) < 0)
			return -ret;

		argc -= 2;
		argv += 2;

		if (argc == 1)
			return EX_OK;
	}

	if (argc != 2) {
		fprintf(stderr, _("usage: %s [-V] [-v] [-o map.orig] map-file\n"),
		        get_progname());
		return EX_USAGE;
	}
	if ((ret = kfont_load_consolemap(kfont, fd, argv[1])) < 0)
		return -ret;

	return EX_OK;
}
