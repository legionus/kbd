/*
 * dumpkeys.c
 *
 * derived from version 0.81 - aeb@cwi.nl
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <getopt.h>
#include <unistd.h>
#include <linux/kd.h>
#include <linux/keyboard.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <sysexits.h>
#include "ksyms.h"
#include "modifiers.h"

#include "libcommon.h"

static int fd;

static void __attribute__((noreturn))
usage(int rc)
{
	fprintf(stderr, _("dumpkeys version %s"), PACKAGE_VERSION);
	fprintf(stderr, _("\n"
	"usage: dumpkeys [options...]\n"
	"\n"
	"Options:\n"
	"  -i, --short-info      display information about keyboard driver;\n"
	"  -l, -s, --long-info   display above and symbols known to loadkeys;\n"
	"  -n, --numeric         display keytable in hexadecimal notation;\n"
	"  -f, --full-table      don't use short-hand notations, one row per keycode;\n"
	"  -1, --separate-lines  one line per (modifier,keycode) pair;\n"
	"  -S, --shape=\n"
	"  -t, --funcs-only      display only the function key strings;\n"
	"  -k, --keys-only       display only key bindings;\n"
	"  -d, --compose-only    display only compose key combinations;\n"
	"  -c, --charset="));
	lk_list_charsets(stderr);
	fprintf(stderr, _(
	"                        interpret character action codes to be from the\n"
	"                        specified character set;\n"));
	fprintf(stderr, _(
	"  -v, --verbose         explain what is being done;\n"
	"  -h, --help            print this usage message;\n"
	"  -V, --version         print version number.\n"));
	exit(rc);
}

int main(int argc, char *argv[])
{
	const char *short_opts          = "hilvsnf1tkdS:c:V";
	const struct option long_opts[] = {
		{ "help", no_argument, NULL, 'h' },
		{ "short-info", no_argument, NULL, 'i' },
		{ "long-info", no_argument, NULL, 'l' },
		{ "numeric", no_argument, NULL, 'n' },
		{ "full-table", no_argument, NULL, 'f' },
		{ "separate-lines", no_argument, NULL, '1' },
		{ "shape", required_argument, NULL, 'S' },
		{ "funcs-only", no_argument, NULL, 't' },
		{ "keys-only", no_argument, NULL, 'k' },
		{ "compose-only", no_argument, NULL, 'd' },
		{ "charset", required_argument, NULL, 'c' },
		{ "verbose", no_argument, NULL, 'v' },
		{ "version", no_argument, NULL, 'V' },
		{ NULL, 0, NULL, 0 }
	};
	int c, rc;
	int kbd_mode;

	char long_info       = 0;
	char short_info      = 0;
	char numeric         = 0;
	lk_table_shape table = LK_SHAPE_DEFAULT;
	char funcs_only      = 0;
	char keys_only       = 0;
	char diac_only       = 0;

	struct lk_ctx *ctx;

	set_progname(argv[0]);
	setuplocale();

	ctx = lk_init();
	if (!ctx) {
		exit(EXIT_FAILURE);
	}

	while ((c = getopt_long(argc, argv,
	                        short_opts, long_opts, NULL)) != -1) {
		switch (c) {
			case 'i':
				short_info = 1;
				break;
			case 's':
			case 'l':
				long_info = 1;
				break;
			case 'n':
				numeric = 1;
				break;
			case 'f':
				table = LK_SHAPE_FULL_TABLE;
				break;
			case '1':
				table = LK_SHAPE_SEPARATE_LINES;
				break;
			case 'S':
				table = atoi(optarg);
				break;
			case 't':
				funcs_only = 1;
				break;
			case 'k':
				keys_only = 1;
				break;
			case 'd':
				diac_only = 1;
				break;
			case 'v':
				lk_set_log_priority(ctx, LOG_INFO);
				break;
			case 'c':
				if ((lk_set_charset(ctx, optarg)) != 0) {
					fprintf(stderr, _("unknown charset %s - ignoring charset request\n"),
					        optarg);
					usage(EX_USAGE);
				}
				printf("charset \"%s\"\n", optarg);
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

	if (optind < argc)
		usage(EX_USAGE);

	if ((fd = getfd(NULL)) < 0)
		kbd_error(EXIT_FAILURE, 0, _("Couldn't get a file descriptor referring to the console"));

	/* check whether the keyboard is in Unicode mode */
	if (ioctl(fd, KDGKBMODE, &kbd_mode)) {
		fprintf(stderr, _("%s: error reading keyboard mode: %m\n"),
		        get_progname());
		exit(EXIT_FAILURE);
	}

	if (kbd_mode == K_UNICODE) {
		lk_set_parser_flags(ctx, LK_FLAG_PREFER_UNICODE);
	}

	if ((rc = lk_kernel_keymap(ctx, fd)) < 0)
		goto fail;

	if (short_info || long_info) {
		lk_dump_summary(ctx, stdout, fd);

		if (long_info) {
			printf(_("Symbols recognized by %s:\n(numeric value, symbol)\n\n"),
			       get_progname());
			lk_dump_symbols(ctx, stdout);
		}
		exit(EXIT_SUCCESS);
	}

#ifdef KDGKBDIACR
	if (!diac_only) {
#endif
		if (!funcs_only) {
			lk_dump_keymap(ctx, stdout, table, numeric);
		}
#ifdef KDGKBDIACR
	}

	if (!funcs_only && !keys_only)
		lk_dump_diacs(ctx, stdout);
#endif

fail:
	lk_free(ctx);
	close(fd);

	if (rc < 0)
		exit(EXIT_FAILURE);

	exit(EXIT_SUCCESS);
}
