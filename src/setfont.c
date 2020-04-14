/*
 * setfont.c - Eugene Crosser & Andries Brouwer
 *
 * Version 1.05
 *
 * Loads the console font, and possibly the corresponding screen map(s).
 * We accept two kind of screen maps, one [-m] giving the correspondence
 * between some arbitrary 8-bit character set currently in use and the
 * font positions, and the second [-u] giving the correspondence between
 * font positions and Unicode values.
 */
#include "config.h"

#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sysexits.h>

#include "libcommon.h"
#include "kfont.h"

static void __attribute__((noreturn))
usage(void)
{
	fprintf(stderr, _(
	                    "Usage: setfont [write-options] [-<N>] [newfont..] [-m consolemap] [-u unicodemap]\n"
	                    "  write-options (take place before file loading):\n"
	                    "    -o  <filename>  Write current font to <filename>\n"
	                    "    -O  <filename>  Write current font and unicode map to <filename>\n"
	                    "    -om <filename>  Write current consolemap to <filename>\n"
	                    "    -ou <filename>  Write current unicodemap to <filename>\n"
	                    "If no newfont and no -[o|O|om|ou|m|u] option is given,\n"
	                    "a default font is loaded:\n"
	                    "    setfont         Load font \"default[.gz]\"\n"
	                    "    setfont -<N>    Load font \"default8x<N>[.gz]\"\n"
	                    "The -<N> option selects a font from a codepage that contains three fonts:\n"
	                    "    setfont -{8|14|16} codepage.cp[.gz]   Load 8x<N> font from codepage.cp\n"
	                    "Explicitly (with -m or -u) or implicitly (in the fontfile) given mappings\n"
	                    "will be loaded and, in the case of consolemaps, activated.\n"
	                    "    -h<N>      (no space) Override font height.\n"
	                    "    -d         Double size of font horizontally and vertically.\n"
	                    "    -m <fn>    Load console screen map.\n"
	                    "    -u <fn>    Load font unicode map.\n"
	                    "    -m none    Suppress loading and activation of a screen map.\n"
	                    "    -u none    Suppress loading of a unicode map.\n"
	                    "    -v         Be verbose.\n"
	                    "    -C <cons>  Indicate console device to be used.\n"
	                    "    -V         Print version and exit.\n"
	                    "Files are loaded from the current directory or %s/*/.\n"),
	        DATADIR);
	exit(EX_USAGE);
}

int main(int argc, char *argv[])
{
	const char *ifiles[MAXIFILES];
	char *mfil, *ufil, *Ofil, *ofil, *omfil, *oufil, *console;
	int ifilct = 0, fd, i, no_m, no_u;
	unsigned int iunit, hwunit;
	int restore = 0;
	int ret;

	set_progname(argv[0]);
	setuplocale();

	struct kfont_context ctx;
	kfont_init(&ctx);

	ifiles[0] = mfil = ufil = Ofil = ofil = omfil = oufil = NULL;
	iunit = hwunit = 0;
	no_m = no_u = 0;
	console     = NULL;

	/*
	 * No getopt() here because of the -om etc options.
	 */
	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-V")) {
			print_version_and_exit();
		} else if (!strcmp(argv[i], "-v")) {
			ctx.verbose++;
		} else if (!strcmp(argv[i], "-R")) {
			restore = 1;
		} else if (!strcmp(argv[i], "-C")) {
			if (++i == argc || console)
				usage();
			console = argv[i];
		} else if (!strcmp(argv[i], "-O")) {
			if (++i == argc || Ofil)
				usage();
			Ofil = argv[i];
		} else if (!strcmp(argv[i], "-o")) {
			if (++i == argc || ofil)
				usage();
			ofil = argv[i];
		} else if (!strcmp(argv[i], "-om")) {
			if (++i == argc || omfil)
				usage();
			omfil = argv[i];
		} else if (!strcmp(argv[i], "-ou")) {
			if (++i == argc || oufil)
				usage();
			oufil = argv[i];
		} else if (!strcmp(argv[i], "-m")) {
			if (++i == argc || mfil)
				usage();
			if (!strcmp(argv[i], "none"))
				no_m = 1;
			else
				mfil = argv[i];
		} else if (!strcmp(argv[i], "-u")) {
			if (++i == argc || ufil)
				usage();
			if (!strcmp(argv[i], "none"))
				no_u = 1;
			else
				ufil = argv[i];
		} else if (!strcmp(argv[i], "-f")) {
			kfont_set_option(&ctx, kfont_force);
		} else if (!strncmp(argv[i], "-h", 2)) {
			int tmp = atoi(argv[i] + 2);
			if (tmp <= 0 || tmp > 32)
				usage();
			hwunit = (unsigned int)tmp;
		} else if (!strcmp(argv[i], "-d")) {
			kfont_set_option(&ctx, kfont_double_size);
		} else if (argv[i][0] == '-') {
			int tmp = atoi(argv[i] + 1);
			if (tmp <= 0 || tmp > 32)
				usage();
			iunit = (unsigned int)tmp;
		} else {
			if (ifilct == MAXIFILES) {
				fprintf(stderr, _("setfont: too many input files\n"));
				exit(EX_USAGE);
			}
			ifiles[ifilct++] = argv[i];
		}
	}

	if (ifilct && restore) {
		fprintf(stderr, _("setfont: cannot both restore from character ROM"
		                  " and from file. Font unchanged.\n"));
		exit(EX_USAGE);
	}

	if ((fd = getfd(console)) < 0)
		kbd_error(EXIT_FAILURE, 0, _("Couldn't get a file descriptor referring to the console"));

	int kd_mode = -1;
	if (!ioctl(fd, KDGETMODE, &kd_mode) && (kd_mode == KD_GRAPHICS)) {
		/*
		 * PIO_FONT will fail on a console which is in foreground and in KD_GRAPHICS mode.
		 * 2005-03-03, jw@suse.de.
		 */
		if (ctx.verbose)
			printf("setfont: graphics console %s skipped\n", console ? console : "");
		close(fd);
		return 0;
	}

	if (!ifilct && !mfil && !ufil &&
	    !Ofil && !ofil && !omfil && !oufil && !restore)
		/* reset to some default */
		ifiles[ifilct++] = "";

	if (Ofil && (ret = kfont_saveoldfontplusunicodemap(&ctx, fd, Ofil)) < 0)
		exit(-ret);

	if (ofil && (ret = kfont_saveoldfont(&ctx, fd, ofil)) < 0)
		exit(-ret);

	if (omfil && (ret = kfont_saveoldmap(&ctx, fd, omfil)) < 0)
		exit(-ret);

	if (oufil && (ret = kfont_saveunicodemap(&ctx, fd, oufil)) < 0)
		exit(-ret);

	if (mfil) {
		if ((ret = kfont_loadnewmap(&ctx, fd, mfil)) < 0)
			exit(-ret);
		kfont_activatemap(fd);
		no_m = 1;
	}

	if (ufil)
		no_u = 1;

	if (restore)
		kfont_restorefont(&ctx, fd);

	if (ifilct)
		kfont_loadnewfonts(&ctx, fd, ifiles, ifilct, iunit, hwunit, no_m, no_u);

	if (ufil && (ret = kfont_loadunicodemap(&ctx, fd, ufil)) < 0)
		return -ret;

	return 0;
}
