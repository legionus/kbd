/* outpsfheader - auxiliary fn - not to be installed */
/* assumes a little-endian machine */
#include "config.h"

#include <stdio.h>
#include <stdlib.h> /* exit */
#include <limits.h>
#include <getopt.h>
#include <sysexits.h>

#include <kfont.h>

#include "libcommon.h"

static void KBD_ATTR_NORETURN
usage(int rc, const struct kbd_help *options)
{
	fprintf(stderr, "Usage: %s [option...] <psftype> <fontsize> <charsize> <hastable>\n",
			program_invocation_short_name);

	print_options(options);
	print_report_bugs();

	exit(rc);
}

int main(int argc, char **argv)
{
	int c, psftype, hastable;
	unsigned int fontsize, charsize;

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

	if ((argc - optind) != 4)
		usage(EX_USAGE, opthelp);

	psftype  = atoi(argv[optind++]);
	fontsize = (unsigned int) atoi(argv[optind++]);
	charsize = (unsigned int) atoi(argv[optind++]);
	hastable = atoi(argv[optind++]);

	if (charsize > UCHAR_MAX)
		kbd_error(EXIT_FAILURE, 0, "charsize is too large");

	if (psftype == 1) {
		struct psf1_header h1;

		if (fontsize != 256 && fontsize != 512) {
			kbd_warning(0, "fontsize can be 256 or 512");
			usage(EX_USAGE, opthelp);
		}

		h1.magic[0] = PSF1_MAGIC0;
		h1.magic[1] = PSF1_MAGIC1;
		h1.mode     = (fontsize == 256) ? 0 : PSF1_MODE512;
		if (hastable)
			h1.mode |= PSF1_MODEHASTAB;
		h1.charsize = (unsigned char) charsize;

		if (fwrite(&h1, sizeof(h1), 1, stdout) != 1)
			kbd_error(EXIT_FAILURE, errno, "fwrite");

	} else if (psftype == 2) {
		struct psf2_header h2;

		h2.magic[0]   = PSF2_MAGIC0;
		h2.magic[1]   = PSF2_MAGIC1;
		h2.magic[2]   = PSF2_MAGIC2;
		h2.magic[3]   = PSF2_MAGIC3;
		h2.version    = 0;
		h2.headersize = sizeof(h2);
		h2.flags      = (hastable ? PSF2_HAS_UNICODE_TABLE : 0);
		h2.length     = fontsize;
		h2.charsize   = charsize;
		h2.width      = 8;
		h2.height     = charsize;

		if (fwrite(&h2, sizeof(h2), 1, stdout) != 1)
			kbd_error(EXIT_FAILURE, errno, "fwrite");

	} else {
		usage(EX_USAGE, opthelp);
	}

	return EXIT_SUCCESS;
}
