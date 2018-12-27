/*
 * loadunimap.c - aeb
 *
 * Version 1.09
 */
#include "config.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/kd.h>

#include <kbdfile.h>
#include <kfont.h>

#include "libcommon.h"
#include "paths.h"

extern char *progname;
extern int force;

static const char *const unidirpath[]  = { "", DATADIR "/" UNIMAPDIR "/", 0 };
static const char *const unisuffixes[] = { "", ".uni", ".sfm", 0 };

int verbose = 0;
int force   = 0;
int debug   = 0;

static void __attribute__((noreturn))
usage(void)
{
	fprintf(stderr,
	        _("Usage:\n\t%s [-C console] [-o map.orig]\n"), progname);
	exit(1);
}

int main(int argc, char *argv[])
{
	int fd, c;
	char *console = NULL;
	char *outfnam = NULL;
	const char *infnam  = "def.uni";

	set_progname(argv[0]);

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE_NAME, LOCALEDIR);
	textdomain(PACKAGE_NAME);

	if (argc == 2 &&
	    (!strcmp(argv[1], "-V") || !strcmp(argv[1], "--version")))
		print_version_and_exit();

	while ((c = getopt(argc, argv, "C:o:")) != EOF) {
		switch (c) {
			case 'C':
				console = optarg;
				break;
			case 'o':
				outfnam = optarg;
				break;
			default:
				usage();
		}
	}

	if (argc > optind + 1 || (argc == optind && !outfnam))
		usage();

	struct kfont_ctx *ctx = kfont_context_new();
	if (ctx == NULL) {
		nomem();
	}

	kfont_set_unidirs(ctx, (char **) unidirpath, (char **) unisuffixes);

	if ((fd = getfd(console)) < 0)
		kbd_error(EXIT_FAILURE, 0, _("Couldn't get a file descriptor referring to the console"));

	kfont_set_console(ctx, fd);

	if (outfnam) {
		if (kfont_dump_unicodemap(ctx, outfnam) < 0)
			exit(EXIT_FAILURE);

		if (argc == optind)
			exit(0);
	}

	if (argc == optind + 1)
		infnam = argv[optind];

	int rc = kfont_load_unicodemap(ctx, infnam);
	if (rc < 0)
		exit(-rc);

	exit(0);
}
