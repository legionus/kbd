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
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/kd.h>
#include <endian.h>
#include <sysexits.h>

#include <kbdfile.h>
#include <kfont.h>

#include "libcommon.h"
#include "paths.h"

extern void activatemap(int fd);
extern void disactivatemap(int fd);

int verbose = 0;
int force   = 0;
int debug   = 0;

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

#define MAXIFILES 256

int main(int argc, char *argv[])
{
	char *ifiles[MAXIFILES];
	char *mfil, *ufil, *Ofil, *ofil, *omfil, *oufil, *console;
	int ifilct  = 0, fd, i, no_m, no_u;
	size_t iunit, hwunit;
	int rc, restore = 0;

	set_progname(argv[0]);

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE_NAME, LOCALEDIR);
	textdomain(PACKAGE_NAME);

	struct kfont_ctx *ctx = kfont_context_new();
	if (ctx == NULL) {
		nomem();
	}

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
			verbose++;
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
			if (!strcmp(argv[i], "none")) {
				kfont_set_flags(ctx, kfont_get_flags(ctx) | KFONT_FLAG_SKIP_LOAD_SCREEN_MAP);
				no_m = 1;
			} else {
				mfil = argv[i];
			}
		} else if (!strcmp(argv[i], "-u")) {
			if (++i == argc || ufil)
				usage();
			if (!strcmp(argv[i], "none")) {
				kfont_set_flags(ctx, kfont_get_flags(ctx) | KFONT_FLAG_SKIP_LOAD_UNICODE_MAP);
				no_u = 1;
			} else {
				ufil = argv[i];
			}
		} else if (!strcmp(argv[i], "-f")) {
			force = 1;
		} else if (!strncmp(argv[i], "-h", 2)) {
			hwunit = atoi(argv[i] + 2);
			if (hwunit <= 0 || hwunit > 32)
				usage();
		} else if (argv[i][0] == '-') {
			iunit = atoi(argv[i] + 1);
			if (iunit <= 0 || iunit > 32)
				usage();
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

	kfont_set_console(ctx, fd);

	int kd_mode = -1;
	if (!ioctl(fd, KDGETMODE, &kd_mode) && (kd_mode == KD_GRAPHICS)) {
		/*
		 * PIO_FONT will fail on a console which is in foreground and in KD_GRAPHICS mode.
		 * 2005-03-03, jw@suse.de.
		 */
		if (verbose)
			printf("setfont: graphics console %s skipped\n", console ? console : "");
		close(fd);
		return 0;
	}

	if (!ifilct && !mfil && !ufil && !Ofil && !ofil && !omfil && !oufil && !restore)
		/* reset to some default */
		ifiles[ifilct++] = "";

	if (Ofil)
		kfont_dump_fullfont(ctx, Ofil);

	if (ofil)
		kfont_dump_font(ctx, ofil);

	if (omfil) {
		if (kfont_dump_map(ctx, omfil) < 0)
			exit(EXIT_FAILURE);
	}

	if (oufil) {
		if (kfont_dump_unicodemap(ctx, oufil) < 0)
			exit(EXIT_FAILURE);
	}

	if (mfil) {
		if (kfont_load_map(ctx, mfil) < 0)
			exit(EXIT_FAILURE);

		activatemap(fd);
		no_m = 1;
	}

	if (ufil)
		no_u = 1;

	if (restore)
		kfont_restore_font(ctx);

	if (ifilct) {
		if (no_m)
			kfont_set_flags(ctx, kfont_get_flags(ctx) | KFONT_FLAG_SKIP_LOAD_SCREEN_MAP);
		if (no_u)
			kfont_set_flags(ctx, kfont_get_flags(ctx) | KFONT_FLAG_SKIP_LOAD_UNICODE_MAP);

		rc = kfont_load_fonts(ctx, ifiles, ifilct, iunit, hwunit);
		if (rc < 0)
			exit(-rc);
	}

	if (ufil) {
		rc = kfont_load_unicodemap(ctx, ufil);
		if (rc < 0)
			exit(-rc);
	}

	return 0;
}

/* Only on the current console? On all allocated consoles? */
/* A newly allocated console has NORM_MAP by default -
   probably it should copy the default from the current console?
   But what if we want a new one because the current one is messed up? */
/* For the moment: only the current console, only the G0 set */

static void
send_escseq(int fd, const char *seq, size_t n)
{
	ssize_t ret = write(fd, seq, n);
	if (ret < 0 || (size_t) ret != n) /* maybe fd is read-only */
		printf("%s", seq);
}

void activatemap(int fd)
{
	send_escseq(fd, "\033(K", 3);
}

void disactivatemap(int fd)
{
	send_escseq(fd, "\033(B", 3);
}
