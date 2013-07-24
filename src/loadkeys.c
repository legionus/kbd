/* loadkeys.c
 *
 * This file is part of kbd project.
 * Copyright (C) 1993  Risto Kankkunen.
 * Copyright (C) 1993  Eugene G. Crosser.
 * Copyright (C) 1994-2007  Andries E. Brouwer.
 * Copyright (C) 2007-2012  Alexey Gladkov <gladkov.alexey@gmail.com>
 *
 * This file is covered by the GNU General Public License,
 * which should be included with kbd as the file COPYING.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "../config.h"
#include "nls.h"
#include "kbd.h"
#include "paths.h"
#include "getfd.h"

#include "keymap.h"

static const char *progname = NULL;
static const char *const dirpath1[] = { "", DATADIR "/" KEYMAPDIR "/**", KERNDIR "/", 0 };
static const char *const suffixes[] = { "", ".kmap", ".map", 0 };

static void __attribute__ ((noreturn))
usage(void)
{
	fprintf(stderr, _("loadkeys version %s\n"
			  "\n"
			  "Usage: %s [option...] [mapfile...]\n"
			  "\n"
			  "Valid options are:\n"
			  "\n"
			  "  -a --ascii         force conversion to ASCII\n"
			  "  -b --bkeymap       output a binary keymap to stdout\n"
			  "  -c --clearcompose  clear kernel compose table\n"
			  "  -C --console=file\n"
			  "                     the console device to be used\n"
			  "  -d --default       load \"%s\"\n"
			  "  -h --help          display this help text\n"
			  "  -m --mktable       output a \"defkeymap.c\" to stdout\n"
			  "  -q --quiet         suppress all normal output\n"
			  "  -s --clearstrings  clear kernel string table\n"
			  "  -u --unicode       force conversion to Unicode\n"
			  "  -v --verbose       report the changes\n"),
		PACKAGE_VERSION, progname, DEFMAP);
	exit(EXIT_FAILURE);
}

static inline const char *
set_progname(const char *name)
{
	char *p;
	p = strrchr(name, '/');
	return (p && p + 1 ? p + 1 : name);
}

int
main(int argc, char *argv[])
{
	const char *const short_opts = "abcC:dhmsuqvV";
	const struct option const long_opts[] = {
		{ "console", required_argument, NULL, 'C'},
		{ "ascii",		no_argument, NULL, 'a' },
		{ "bkeymap",		no_argument, NULL, 'b' },
		{ "clearcompose",	no_argument, NULL, 'c' },
		{ "default",		no_argument, NULL, 'd' },
		{ "help",		no_argument, NULL, 'h' },
		{ "mktable",		no_argument, NULL, 'm' },
		{ "clearstrings",	no_argument, NULL, 's' },
		{ "unicode",		no_argument, NULL, 'u' },
		{ "quiet",		no_argument, NULL, 'q' },
		{ "verbose",		no_argument, NULL, 'v' },
		{ "version",		no_argument, NULL, 'V' },
		{ NULL, 0, NULL, 0 }
	};

	enum options {
		OPT_A = (1 << 1),
		OPT_B = (1 << 2),
		OPT_D = (1 << 3),
		OPT_M = (1 << 4),
		OPT_U = (1 << 5)
	};
	int options = 0;

	const char *const *dirpath;
	const char *dirpath2[] = { 0, 0 };

	struct lk_ctx *ctx;
	lk_flags flags = 0;

	int c, i, rc = -1;
	int fd;
	int kbd_mode;
	int kd_mode;
	char *console = NULL;
	char *ev;
	lkfile_t f;

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE_NAME, LOCALEDIR);
	textdomain(PACKAGE_NAME);

	progname = set_progname(argv[0]);

	ctx = lk_init();
	if (!ctx) {
		exit(EXIT_FAILURE);
	}

	while ((c = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
		switch (c) {
		case 'a':
			options |= OPT_A;
			break;
		case 'b':
			options |= OPT_B;
			break;
		case 'c':
			flags |= LK_FLAG_CLEAR_COMPOSE;
			break;
		case 'C':
			console = optarg;
			break;
		case 'd':
			options |= OPT_D;
			break;
		case 'm':
			options |= OPT_M;
			break;
		case 's':
			flags |= LK_FLAG_CLEAR_STRINGS;
			break;
		case 'u':
			options |= OPT_U;
			flags |= LK_FLAG_UNICODE_MODE;
			flags |= LK_FLAG_PREFER_UNICODE;
			break;
		case 'q':
			lk_set_log_priority(ctx, LOG_ERR);
			break;
		case 'v':
			lk_set_log_priority(ctx, LOG_INFO);
			break;
		case 'V':
			fprintf(stdout, _("%s from %s\n"), progname, PACKAGE_STRING);
			exit(0);
		case 'h':
		case '?':
			usage();
		}
	}

	if ((options & OPT_U) && (options & OPT_A)) {
		fprintf(stderr,
			_("%s: Options --unicode and --ascii are mutually exclusive\n"),
			progname);
		exit(EXIT_FAILURE);
	}

	/* get console */
	fd = getfd(console);

	if (!(options & OPT_M) && !(options & OPT_B)) {
		/* check whether the keyboard is in Unicode mode */
		if (ioctl(fd, KDGKBMODE, &kbd_mode) ||
		    ioctl(fd, KDGETMODE, &kd_mode)) {
			fprintf(stderr, _("%s: error reading keyboard mode: %m\n"),
				progname);
			exit(EXIT_FAILURE);
		}

		if (kbd_mode == K_UNICODE) {
			if (options & OPT_A) {
				fprintf(stderr,
					_("%s: warning: loading non-Unicode keymap on Unicode console\n"
					  "    (perhaps you want to do `kbd_mode -a'?)\n"),
					progname);
			} else {
				flags |= LK_FLAG_PREFER_UNICODE;
			}

			/* reset -u option if keyboard is in K_UNICODE anyway */
			flags ^= LK_FLAG_UNICODE_MODE;

		} else if (options & OPT_U && kd_mode != KD_GRAPHICS) {
			fprintf(stderr,
				_("%s: warning: loading Unicode keymap on non-Unicode console\n"
				  "    (perhaps you want to do `kbd_mode -u'?)\n"),
				progname);
		}
	}

	lk_set_parser_flags(ctx, flags);

	dirpath = dirpath1;
	if ((ev = getenv("LOADKEYS_KEYMAP_PATH")) != NULL) {
		dirpath2[0] = ev;
		dirpath = dirpath2;
	}

	if (options & OPT_D) {
		/* first read default map - search starts in . */

		if (lk_findfile(DEFMAP, dirpath, suffixes, &f)) {
			fprintf(stderr, _("Cannot find %s\n"), DEFMAP);
			exit(EXIT_FAILURE);
		}

		if ((rc = lk_parse_keymap(ctx, &f)) == -1)
			goto fail;


	} else if (optind == argc) {
		f.fd = stdin;
		strcpy(f.pathname, "<stdin>");

		if ((rc = lk_parse_keymap(ctx, &f)) == -1)
			goto fail;
	}

	for (i = optind; argv[i]; i++) {
		if (!strcmp(argv[i], "-")) {
			f.fd = stdin;
			strcpy(f.pathname, "<stdin>");

		} else if (lk_findfile(argv[i], dirpath, suffixes, &f)) {
			fprintf(stderr, _("cannot open file %s\n"), argv[i]);
			goto fail;
		}

		if ((rc = lk_parse_keymap(ctx, &f)) == -1)
			goto fail;
	}

	if (options & OPT_B) {
		rc = lk_dump_bkeymap(ctx, stdout);
	} else if (options & OPT_M) {
		rc = lk_dump_ctable(ctx, stdout);
	} else {
		rc = lk_load_keymap(ctx, fd, kbd_mode);
	}

 fail:	lk_free(ctx);
	lk_fpclose(&f);
	close(fd);

	if (rc < 0)
		exit(EXIT_FAILURE);

	exit(EXIT_SUCCESS);
}
