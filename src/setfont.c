/*
 * setfont.c - Eugene Crosser & Andries Brouwer
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
usage(const struct kbd_help *options)
{
	fprintf(stderr, _("Usage: %s [option...] [newfont...]\n"), get_progname());
	fprintf(stderr, "\n");
	fprintf(stderr, _("Loads the console font, and possibly the corresponding screen map(s).\n"));

	print_options(options);
	fprintf(stderr, "\n");

	fprintf(stderr,
		_("The options -[o|O|om|ou] are processed before the new font is uploaded.\n"
		  "\n"
		  "If no <newfont> and no -[o|O|om|ou|m|u] option is given, a default\n"
		  "font is loaded.\n"
		  "\n"
		  "There are two kinds of screen maps, one [-m] giving the correspondence\n"
		  "between some arbitrary 8-bit character set currently in use and the\n"
		  "font positions, and the second [-u] giving the correspondence between\n"
		  "font positions and Unicode values.\n"
		  "\n"
		  "Explicitly (with -m or -u) or implicitly (in the fontfile) given\n"
		  "mappings will be loaded and, in the case of consolemaps, activated.\n"
		  "\n"
		  "Files are loaded from the %s/*/.\n"),
		DATADIR);

	print_report_bugs();

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

	struct kfont_context *kfont;

	const struct kbd_help opthelp[] = {
		{ "-<N>",       _("load font \"default8x<N>\".") },
		{ "-o <FILE>",  _("write current font to <FILE>.") },
		{ "-O <FILE>",  _("write current font and unicode map to <FILE>.") },
		{ "-om <FILE>", _("write current consolemap to <FILE>.") },
		{ "-ou <FILE>", _("write current unicodemap to <FILE>.") },
		{ "-h<N>",      _("override font height.") },
		{ "-d",         _("double size of font horizontally and vertically.") },
		{ "-m <FILE>",  _("load console screen map ('none' means don't load it).") },
		{ "-u <FILE>",  _("load font unicode map ('none' means don't load it).") },
		{ "-R",         _("reset the screen font, size, and unicode map to the bootup defaults.") },
		{ "-C <DEV>",   _("the console device to be used.") },
		{ "-v",         _("be more verbose.") },
		{ "-V",         _("print version number.") },
		{ NULL, NULL }
	};

	set_progname(argv[0]);
	setuplocale();

	if ((ret = kfont_init(get_progname(), &kfont)) < 0)
		return -ret;

	ifiles[0] = mfil = ufil = Ofil = ofil = omfil = oufil = NULL;
	iunit = hwunit = 0;
	no_m = no_u = 0;
	console = NULL;

	/*
	 * No getopt() here because of the -om etc options.
	 */
	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-V")) {
			print_version_and_exit();
		} else if (!strcmp(argv[i], "-v")) {
			kfont_inc_verbosity(kfont);
		} else if (!strcmp(argv[i], "-R")) {
			restore = 1;
		} else if (!strcmp(argv[i], "-C")) {
			if (++i == argc || console)
				usage(opthelp);
			console = argv[i];
		} else if (!strcmp(argv[i], "-O")) {
			if (++i == argc || Ofil)
				usage(opthelp);
			Ofil = argv[i];
		} else if (!strcmp(argv[i], "-o")) {
			if (++i == argc || ofil)
				usage(opthelp);
			ofil = argv[i];
		} else if (!strcmp(argv[i], "-om")) {
			if (++i == argc || omfil)
				usage(opthelp);
			omfil = argv[i];
		} else if (!strcmp(argv[i], "-ou")) {
			if (++i == argc || oufil)
				usage(opthelp);
			oufil = argv[i];
		} else if (!strcmp(argv[i], "-m")) {
			if (++i == argc || mfil)
				usage(opthelp);
			if (!strcmp(argv[i], "none"))
				no_m = 1;
			else
				mfil = argv[i];
		} else if (!strcmp(argv[i], "-u")) {
			if (++i == argc || ufil)
				usage(opthelp);
			if (!strcmp(argv[i], "none"))
				no_u = 1;
			else
				ufil = argv[i];
		} else if (!strcmp(argv[i], "-f")) {
			kfont_set_option(kfont, kfont_force);
		} else if (!strncmp(argv[i], "-h", 2)) {
			int tmp = atoi(argv[i] + 2);
			if (tmp <= 0 || tmp > 32)
				usage(opthelp);
			hwunit = (unsigned int)tmp;
		} else if (!strcmp(argv[i], "-d")) {
			kfont_set_option(kfont, kfont_double_size);
		} else if (argv[i][0] == '-') {
			int tmp = atoi(argv[i] + 1);
			if (tmp <= 0 || tmp > 32)
				usage(opthelp);
			iunit = (unsigned int)tmp;
		} else {
			if (ifilct == MAXIFILES)
				kbd_error(EX_USAGE, 0, _("Too many input files."));
			ifiles[ifilct++] = argv[i];
		}
	}

	if (ifilct && restore)
		kbd_error(EX_USAGE, 0, _("Cannot both restore from character ROM"
					 " and from file. Font unchanged."));

	if ((fd = getfd(console)) < 0)
		kbd_error(EX_OSERR, 0, _("Couldn't get a file descriptor referring to the console."));

	int kd_mode = -1;
	if (!ioctl(fd, KDGETMODE, &kd_mode) && (kd_mode == KD_GRAPHICS)) {
		/*
		 * PIO_FONT will fail on a console which is in foreground and in KD_GRAPHICS mode.
		 * 2005-03-03, jw@suse.de.
		 */
		if (kfont_get_verbosity(kfont))
			kbd_warning(0, "graphics console %s skipped", console ? console : "");
		close(fd);
		return EX_OK;
	}

	if (!ifilct && !mfil && !ufil &&
	    !Ofil && !ofil && !omfil && !oufil && !restore)
		/* reset to some default */
		ifiles[ifilct++] = "";

	if (Ofil && (ret = kfont_save_font(kfont, fd, Ofil, 1)) < 0)
		return -ret;

	if (ofil && (ret = kfont_save_font(kfont, fd, ofil, 0)) < 0)
		return -ret;

	if (omfil && (ret = kfont_save_consolemap(kfont, fd, omfil)) < 0)
		return -ret;

	if (oufil && (ret = kfont_save_unicodemap(kfont, fd, oufil)) < 0)
		return -ret;

	if (mfil) {
		if ((ret = kfont_load_consolemap(kfont, fd, mfil)) < 0)
			return -ret;
		kfont_activatemap(fd);
		no_m = 1;
	}

	if (ufil)
		no_u = 1;

	if (restore)
		kfont_restore_font(kfont, fd);

	if (ifilct && (ret = kfont_load_fonts(kfont, fd, ifiles, ifilct, iunit, hwunit, no_m, no_u)) < 0)
		return -ret;

	if (ufil && (ret = kfont_load_unicodemap(kfont, fd, ufil)) < 0)
		return -ret;

	return EX_OK;
}
