/*
 * loadunimap.c - aeb
 *
 * Version 1.09
 */
#include "config.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <sysexits.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/kd.h>

#include <kfont.h>

#include "libcommon.h"

static void KBD_ATTR_NORETURN
usage(int rc, const struct kbd_help *options)
{
	fprintf(stderr, _("Usage: %s [option...]\n"), program_invocation_short_name);
	fprintf(stderr, "\n");
	fprintf(stderr, _("This utility reports or sets the keyboard mode.\n"));

	print_options(options);
	print_report_bugs();

	exit(rc);
}

int main(int argc, char *argv[])
{
	int fd, c, ret, rc;
	char *console = NULL;
	char *outfnam = NULL;
	const char *infnam  = "def.uni";

	setuplocale();

	const char *short_opts = "o:C:hV";
	const struct option long_opts[] = {
		{ "output",   required_argument, NULL, 'o' },
		{ "console",  required_argument, NULL, 'C' },
		{ "help",     no_argument,       NULL, 'h' },
		{ "version",  no_argument,       NULL, 'V' },
		{ NULL,       0,                 NULL,  0  }
	};
	const struct kbd_help opthelp[] = {
		{ "-o, --output=FILE", _("save the old map to the FILE.") },
		{ "-C, --console=DEV", _("the console device to be used.") },
		{ "-V, --version",     _("print version number.")     },
		{ "-h, --help",        _("print this usage message.") },
		{ NULL, NULL }
	};

	while ((c = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
		switch (c) {
			case 'o':
				outfnam = optarg;
				break;
			case 'C':
				if (optarg == NULL || optarg[0] == '\0')
					usage(EX_USAGE, opthelp);
				console = optarg;
				break;
			case 'V':
				print_version_and_exit();
				break;
			case 'h':
				usage(EXIT_SUCCESS, opthelp);
				break;
			case '?':
				usage(EX_USAGE, opthelp);
				break;
		}
	}

	if (argc > optind + 1 || (argc == optind && !outfnam))
		usage(EX_USAGE, opthelp);

	if ((fd = getfd(console)) < 0)
		kbd_error(EXIT_FAILURE, 0, _("Couldn't get a file descriptor referring to the console."));

	struct kfont_context *kfont;

	if ((ret = kfont_init(program_invocation_short_name, &kfont)) < 0)
		return -ret;

	rc = EX_OK;

	if (outfnam) {
		if ((ret = kfont_save_unicodemap(kfont, fd, outfnam)) < 0) {
			rc = -ret;
			goto kfont_exit;
		}
		if (argc == optind)
			goto kfont_exit;
	}

	if (argc == optind + 1)
		infnam = argv[optind];

	if ((ret = kfont_load_unicodemap(kfont, fd, infnam)) < 0)
		rc = -ret;

kfont_exit:
	kfont_free(kfont);

	return rc;
}
