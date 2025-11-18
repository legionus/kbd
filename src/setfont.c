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

#include <kfont.h>

#include "libcommon.h"

static void KBD_ATTR_NORETURN
usage(int retcode, const struct kbd_help *options)
{
	fprintf(stderr, _("Usage: %s [option...] [newfont...]\n"), program_invocation_short_name);
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

	exit(retcode);
}

enum kbd_getopt_arg {
	kbd_no_argument,
	kbd_required_argument
};

struct kbd_option {
	const char *short_name;
	const char *long_name;
	enum kbd_getopt_arg has_arg;
	int val;
};

static int
kbd_getopt(int argc, char **argv, const struct kbd_option *opts)
{
	const struct kbd_option *opt;
	char *p, *o, *option;
	const char *name;

	optarg = NULL;

	if (!optind)
		optind = 1;

	if (optind >= argc || !argv[optind])
		return -1;

	if (argv[optind][0] != '-')
		return '?';

	option = argv[optind];

	if (option[1] && option[1] != '-') {
		// A short option that starts with "-" (not "--"). Unlike the
		// standard getopt for historical reasons we have two character
		// short options.
		option += 1;

		for (opt = opts; opt->val; opt++) {
			name = opt->short_name + 1;

			for (p = (char *) name, o = option; *p && *p == *o; p++, o++);

			switch (opt->short_name[0]) {
				// Case: full match option.
				case '=':
					if (*p == *o) {
						optind++;

						if (opt->has_arg) {
							if (optind == argc)
								goto required_argument;
							optarg = argv[optind++];
						}
						return opt->val;
					}
					break;
				// Case: prefix option.
				case '+':
					if (*p == '\0' && *o != '\0') {
						optind++;

						if (opt->has_arg)
							optarg = o;
						return opt->val;
					}
					break;
			}
		}
	} else if (option[1] && option[1] == '-') {
		// The special argument "--" forces an end of option-scanning
		// regardless of the scanning mode.
		if (!option[2]) {
			optind++;
			return -1;
		}
		option += 2;

		for (opt = opts; opt->val; opt++) {
			name = opt->long_name;

			for (p = (char *) name, o = option; *p && *p == *o; p++, o++);

			// Case: --arg param
			if (*p == *o) {
				optind++;

				if (opt->has_arg) {
					if (optind == argc)
						goto required_argument;
					optarg = argv[optind++];
				}
				return opt->val;
			}

			if (!opt->has_arg)
				continue;

			// Case: --arg=param
			if (*p == '\0' && *o == '=') {
				optind++;
				optarg = ++o;

				if (!optarg[0])
					goto required_argument;
				return opt->val;
			}
		}
	}
	return '?';

required_argument:
	optind--;
	return '!';
}

int main(int argc, char *argv[])
{
	const char *ifiles[MAXIFILES];
	char *mfil, *ufil, *Ofil, *ofil, *omfil, *oufil, *console;
	int ifilct = 0, fd, no_m, no_u;
	unsigned int iunit, hwunit;
	int restore = 0;
	int ret, c;

	struct kfont_context *kfont;

	const struct kbd_help opthelp[] = {
		{ "-<N>, --default8x <N>",           _("load font \"default8x<N>\".") },
		{ "-h<N>, --font-height <N>",        _("override font height (there shouldn't be a space in the short option).") },
		{ "-o, --output-font <FILE>",        _("write current font to <FILE>.") },
		{ "-om, --output-consolemap <FILE>", _("write current consolemap to <FILE>.") },
		{ "-ou, --output-unicodemap <FILE>", _("write current unicodemap to <FILE>.") },
		{ "-O, --output-fullfont <FILE>",    _("write current font and unicode map to <FILE>.") },
		{ "-m, --consolemap <FILE>",         _("load console screen map ('none' means don't load it).") },
		{ "-u, --unicodemap <FILE>",         _("load font unicode map ('none' means don't load it).") },
		{ "-C, --console <DEV>",             _("the console device to be used.") },
		{ "-d, --double",                    _("double size of font horizontally and vertically.") },
		{ "-f, --force",                     _("force load unicode map.") },
		{ "-R, --reset",                     _("reset the screen font, size, and unicode map to the bootup defaults.") },
		{ "-v, --verbose",                   _("be more verbose.") },
		{ "-V, --version",                   _("print version number.") },
		{ "-h, --help",                      _("print this usage message.") },
		{ NULL, NULL }
	};

	const struct kbd_option opts[] = {
		{ "=d",  "double",            kbd_no_argument,       'd' },
		{ "=f",  "force",             kbd_no_argument,       'f' },
		{ "=R",  "reset",             kbd_no_argument,       'R' },
		{ "=v",  "verbose",           kbd_no_argument,       'v' },
		{ "=V",  "version",           kbd_no_argument,       'V' },
		{ "=h",  "help",              kbd_no_argument,       'H' },
		{ "+h",  "font-height",       kbd_required_argument, 'h' },
		{ "=om", "output-consolemap", kbd_required_argument, 'M' },
		{ "=ou", "output-unicodemap", kbd_required_argument, 'U' },
		{ "=o",  "output-font",       kbd_required_argument, 'o' },
		{ "=O",  "output-fullfont",   kbd_required_argument, 'O' },
		{ "=m",  "consolemap",        kbd_required_argument, 'm' },
		{ "=u",  "unicodemap",        kbd_required_argument, 'u' },
		{ "=C",  "console",           kbd_required_argument, 'C' },
		{ "+",   "default8x",         kbd_required_argument, 'N' },
		{ NULL, NULL, 0, 0 },
	};

	setuplocale();

	if ((ret = kfont_init(program_invocation_short_name, &kfont)) < 0)
		return -ret;

	ifiles[0] = mfil = ufil = Ofil = ofil = omfil = oufil = NULL;
	iunit = hwunit = 0;
	no_m = no_u = 0;
	console = NULL;

	while ((c = kbd_getopt(argc, argv, opts)) != -1) {
		switch (c) {
			case 'h': {
					int num = atoi(optarg);
					if (num <= 0 || num > 32) {
						kbd_warning(0, "bad option '%s': not a number '%s'",
							    argv[optind - 1], optarg);
						usage(EX_USAGE, opthelp);
					}
					hwunit = (unsigned int) num;
				}
				break;
			case 'N': {
					int num = atoi(optarg);
					if (num <= 0 || num > 32) {
						kbd_warning(0, "unrecognized option '%s'",
							    argv[optind - 1]);
						usage(EX_USAGE, opthelp);
					}
					iunit = (unsigned int) num;
				}
				break;
			case 'o':
				ofil = optarg;
				break;
			case 'M':
				omfil = optarg;
				break;
			case 'U':
				oufil = optarg;
				break;
			case 'm':
				no_m = !strcmp(optarg, "none");
				if (!no_m)
					mfil = optarg;
				break;
			case 'u':
				no_u = !strcmp(optarg, "none");
				if (!no_u)
					ufil = optarg;
				break;
			case 'O':
				Ofil = optarg;
				break;
			case 'C':
				console = optarg;
				break;
			case 'R':
				restore = 1;
				break;
			case 'd':
				kfont_set_option(kfont, kfont_double_size);
				break;
			case 'f':
				kfont_set_option(kfont, kfont_force);
				break;
			case 'v':
				kfont_inc_verbosity(kfont);
				break;
			case 'V':
				print_version_and_exit();
				break;
			case 'H':
				usage(EXIT_SUCCESS, opthelp);
				break;
			case '?':
				if (argv[optind][0] == '-') {
					kbd_warning(0, "invalid option '%s'", argv[optind]);
					usage(EX_USAGE, opthelp);
				}

				if (ifilct == MAXIFILES)
					kbd_error(EX_USAGE, 0, _("Too many input files."));
				ifiles[ifilct++] = argv[optind++];
				break;
			case '!':
				kbd_warning(0, "option '%s' requires an argument", argv[optind]);
				usage(EX_USAGE, opthelp);
				break;
		}
	}

	for (; optind < argc; optind++) {
		if (ifilct == MAXIFILES)
			kbd_error(EX_USAGE, 0, _("Too many input files."));
		ifiles[ifilct++] = argv[optind];
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
