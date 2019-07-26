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
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <sysexits.h>
#include <sys/ioctl.h>

#include "libcommon.h"

#include "paths.h"
#include "keymap.h"

static const char *const dirpath1[] = { "", DATADIR "/" KEYMAPDIR "/**", KERNDIR "/", 0 };
static const char *const suffixes[] = { "", ".kmap", ".map", 0 };

static void __attribute__((noreturn))
usage(int rc)
{
	fprintf(stderr, _(
		"loadkeys version %s\n"
		"\n"
		"Usage: %s [option...] [mapfile...]\n"
		"\n"
		"Options:\n"
		"  -a, --ascii           force conversion to ASCII;\n"
		"  -b, --bkeymap         output a binary keymap to stdout;\n"
		"  -c, --clearcompose    clear kernel compose table;\n"
		"  -C, --console=file    the console device to be used;\n"
		"  -d, --default         load \"%s\";\n"
		"  -m, --mktable         output a \"defkeymap.c\" to stdout;\n"
		"  -p, --parse           search and parse keymap without action;\n"
		"  -s, --clearstrings    clear kernel string table;\n"
		"  -u, --unicode         force conversion to Unicode;\n"
		"  -q, --quiet           suppress all normal output;\n"
		"  -v, --verbose         explain what is being done;\n"
		"  -h, --help            print this usage message;\n"
		"  -V, --version         print version number.\n"),
	        PACKAGE_VERSION, get_progname(), DEFMAP);
	exit(rc);
}

int main(int argc, char *argv[])
{
	const char *const short_opts          = "abcC:dhmpsuqvV";
	const struct option long_opts[] = {
		{ "console", required_argument, NULL, 'C' },
		{ "ascii", no_argument, NULL, 'a' },
		{ "bkeymap", no_argument, NULL, 'b' },
		{ "clearcompose", no_argument, NULL, 'c' },
		{ "default", no_argument, NULL, 'd' },
		{ "help", no_argument, NULL, 'h' },
		{ "mktable", no_argument, NULL, 'm' },
		{ "parse", no_argument, NULL, 'p' },
		{ "clearstrings", no_argument, NULL, 's' },
		{ "unicode", no_argument, NULL, 'u' },
		{ "quiet", no_argument, NULL, 'q' },
		{ "verbose", no_argument, NULL, 'v' },
		{ "version", no_argument, NULL, 'V' },
		{ NULL, 0, NULL, 0 }
	};

	enum options {
		OPT_A = (1 << 1),
		OPT_B = (1 << 2),
		OPT_D = (1 << 3),
		OPT_M = (1 << 4),
		OPT_U = (1 << 5),
		OPT_P = (1 << 6)
	};
	int options = 0;

	const char *const *dirpath;
	const char *dirpath2[] = { 0, 0 };

	struct lk_ctx *ctx;
	lk_flags flags = 0;

	int c, i, rc = -1;
	int fd = -1;
	int kbd_mode;
	int kd_mode;
	char *console = NULL;
	char *ev;
	struct kbdfile_ctx *fctx;
	struct kbdfile *fp = NULL;

	set_progname(argv[0]);
	setuplocale();

	ctx = lk_init();
	if (!ctx) {
		exit(EXIT_FAILURE);
	}

	if ((fctx = kbdfile_context_new()) == NULL)
		nomem();

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
			case 'p':
				options |= OPT_P;
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
				print_version_and_exit();
				break;
			case 'h':
				usage(EXIT_SUCCESS);
				break;
			case '?':
				usage(EX_USAGE);
				break;
		}
	}

	if ((options & OPT_U) && (options & OPT_A)) {
		fprintf(stderr,
		        _("%s: Options --unicode and --ascii are mutually exclusive\n"),
		        get_progname());
		exit(EXIT_FAILURE);
	}

	if (!(options & OPT_M) && !(options & OPT_B)) {
		/* get console */
		if ((fd = getfd(console)) < 0)
			kbd_error(EXIT_FAILURE, 0, _("Couldn't get a file descriptor referring to the console"));

		/* check whether the keyboard is in Unicode mode */
		if (ioctl(fd, KDGKBMODE, &kbd_mode) ||
		    ioctl(fd, KDGETMODE, &kd_mode)) {
			fprintf(stderr, _("%s: error reading keyboard mode: %m\n"),
			        get_progname());
			exit(EXIT_FAILURE);
		}

		if (kbd_mode == K_UNICODE) {
			if (options & OPT_A) {
				fprintf(stderr,
				        _("%s: warning: loading non-Unicode keymap on Unicode console\n"
				          "    (perhaps you want to do `kbd_mode -a'?)\n"),
				        get_progname());
			} else {
				flags |= LK_FLAG_PREFER_UNICODE;
			}

			/* reset -u option if keyboard is in K_UNICODE anyway */
			flags ^= LK_FLAG_UNICODE_MODE;

		} else if (options & OPT_U && kd_mode != KD_GRAPHICS) {
			fprintf(stderr,
			        _("%s: warning: loading Unicode keymap on non-Unicode console\n"
			          "    (perhaps you want to do `kbd_mode -u'?)\n"),
			        get_progname());
		}
	}

	lk_set_parser_flags(ctx, flags);

	dirpath = dirpath1;
	if ((ev = getenv("LOADKEYS_KEYMAP_PATH")) != NULL) {
		dirpath2[0] = ev;
		dirpath     = dirpath2;
	}

	if (options & OPT_D) {
		if ((fp = kbdfile_new(fctx)) == NULL)
			nomem();

		/* first read default map - search starts in . */
		if (kbdfile_find((char *) DEFMAP, dirpath, suffixes, fp)) {
			fprintf(stderr, _("Cannot find %s\n"), DEFMAP);
			exit(EXIT_FAILURE);
		}

		rc = lk_parse_keymap(ctx, fp);
		kbdfile_free(fp);

		if (rc == -1)
			goto fail;

	} else if (optind == argc) {
		if ((fp = kbdfile_new(fctx)) == NULL)
			nomem();

		kbdfile_set_file(fp, stdin);
		kbdfile_set_pathname(fp, "<stdin>");

		rc = lk_parse_keymap(ctx, fp);
		kbdfile_free(fp);

		if (rc == -1)
			goto fail;
	}

	for (i = optind; argv[i]; i++) {
		if ((fp = kbdfile_new(fctx)) == NULL)
			nomem();

		if (!strcmp(argv[i], "-")) {
			kbdfile_set_file(fp, stdin);
			kbdfile_set_pathname(fp, "<stdin>");

		} else if (kbdfile_find(argv[i], dirpath, suffixes, fp)) {
			fprintf(stderr, _("cannot open file %s\n"), argv[i]);
			goto fail;
		}

		rc = lk_parse_keymap(ctx, fp);
		kbdfile_free(fp);

		if (rc == -1)
			goto fail;
	}

	if (!(options & OPT_P)) {
		if (options & OPT_B) {
			rc = lk_dump_bkeymap(ctx, stdout);
		} else if (options & OPT_M) {
			rc = lk_dump_ctable(ctx, stdout);
		} else {
			rc = lk_load_keymap(ctx, fd, kbd_mode);
		}
	}

fail:
	lk_free(ctx);
	kbdfile_context_free(fctx);

	if (fd >= 0)
		close(fd);

	if (rc < 0)
		exit(EXIT_FAILURE);

	exit(EXIT_SUCCESS);
}
