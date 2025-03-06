/*
 * clrunimap.c
 *
 * Note: nowadays this kills kernel console output!
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <getopt.h>
#include <sysexits.h>
#include <linux/kd.h>

#include <kfont.h>

#include "libcommon.h"

static void KBD_ATTR_NORETURN
usage(int rc, const struct kbd_help *options)
{
	fprintf(stderr, "Usage: %s [option...]\n", program_invocation_short_name);

	print_options(options);
	print_report_bugs();

	exit(rc);
}

int main(int argc, char *argv[])
{
	int c, fd;
	char *console = NULL;

	const char *short_opts = "C:hV";
	const struct option long_opts[] = {
		{ "console", required_argument, NULL, 'C' },
		{ "help",    no_argument,       NULL, 'h' },
		{ "version", no_argument,       NULL, 'V' },
		{ NULL,      0,                 NULL,  0  }
	};
	const struct kbd_help opthelp[] = {
		{ "-C, --console=DEV", _("the console device to be used.") },
		{ "-V, --version",     _("print version number.")     },
		{ "-h, --help",        _("print this usage message.") },
		{ NULL, NULL }
	};

	setuplocale();

	while ((c = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
		switch (c) {
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

	if ((fd = getfd(console)) < 0)
		kbd_error(EXIT_FAILURE, 0, _("Couldn't get a file descriptor referring to the console."));

	int ret;
	struct kfont_context *ctx;

	if ((ret = kfont_init(program_invocation_short_name, &ctx)) < 0)
		return -ret;

	ret = EXIT_SUCCESS;

	if (kfont_put_unicodemap(ctx, fd, NULL, NULL) < 0)
		ret = EXIT_FAILURE;

	kfont_free(ctx);

	return ret;
}
