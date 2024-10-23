#include "config.h"

#include <sysexits.h>
#include <stdio.h>
#include <stdlib.h> /* exit */
#include <getopt.h>

#include <kfont.h>

#include "libcommon.h"

static void KBD_ATTR_NORETURN
usage(int rc, const struct kbd_help *options)
{
	fprintf(stderr, "Usage: %s [option...] <psffile>\n",
			program_invocation_short_name);

	print_options(options);
	print_report_bugs();

	exit(rc);
}

int main(int argc, char **argv)
{
	int c;
	int psftype, fontlen, charsize, hastable, notable;
	int width = 8, bytewidth, height;
	char *inbuf, *fontbuf;
	int inbuflth, fontbuflth;
	struct unicode_list *uclistheads = NULL;

	const char *short_opts = "hV";
	const struct option long_opts[] = {
		{ "help",    no_argument,       NULL, 'h' },
		{ "version", no_argument,       NULL, 'V' },
		{ NULL,      0,                 NULL,  0  }
	};
	const struct kbd_help opthelp[] = {
		{ "-V, --version",     _("print version number.")     },
		{ "-h, --help",        _("print this usage message.") },
		{ NULL, NULL }
	};

	setuplocale();

	while ((c = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
		switch (c) {
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

	if (optind == argc)
		usage(EX_USAGE, opthelp);

	FILE *f = fopen(argv[optind], "rb");
	if (!f) {
		perror("fopen");
		return EX_NOINPUT;
	}

	int ret;
	struct kfont_context *kfont;

	if ((ret = kfont_init(program_invocation_short_name, &kfont)) < 0)
		return -ret;

	if (kfont_read_psffont(kfont, f, &inbuf, &inbuflth, &fontbuf, &fontbuflth, &width, &fontlen, 0, &uclistheads) < 0)
		kbd_error(EX_DATAERR, 0, "Bad magic number");

	close(f);

	return EX_OK;
}
