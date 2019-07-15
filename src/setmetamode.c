/*
 * setmetamode.c - aeb, 940130
 *
 * Call: setmetamode { metabit | escprefix }
 * and report the setting before and after.
 * Without arguments setmetamode will only report.
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <sysexits.h>
#include <linux/kd.h>
#include <sys/ioctl.h>

#include "libcommon.h"

static void __attribute__((noreturn))
usage(int rc)
{
	fprintf(stderr, _("Usage: %1$s [option...] [argument]\n"
	                  "\n"
	                  "Each vt has his own copy of this bit. Use\n"
	                  "    %1$s [argument] < /dev/ttyn\n"
	                  "to change the settings of another vt.\n"
	                  "The setting before and after the change are reported.\n"
	                  "\n"
	                  "Arguments:\n"
	                  "  metabit     the keysym marked with the high bit set.\n"
	                  "  escprefix   specifies if pressing the meta (alt) key\n"
	                  "              generates an ESC (\\033) prefix followed by\n"
	                  "              the keysym.\n"
	                  "\n"
	                  "Options:\n"
	                  "  -C, --console=DEV     the console device to be used;\n"
	                  "  -h, --help            print this usage message;\n"
	                  "  -V, --version         print version number\n"
	                  "\n"), get_progname());
	exit(rc);
}

static void
report(unsigned int meta)
{
	char *s;

	switch (meta) {
		case K_METABIT:
			s = _("Meta key sets high order bit\n");
			break;
		case K_ESCPREFIX:
			s = _("Meta key gives Esc prefix\n");
			break;
		default:
			s = _("Strange mode for Meta key?\n");
	}
	printf("%s", s);
}

struct meta {
	const char *name;
	unsigned int val;
} metas[] = {
	{ "metabit", K_METABIT },
	{ "meta", K_METABIT },
	{ "bit", K_METABIT },
	{ "escprefix", K_ESCPREFIX },
	{ "esc", K_ESCPREFIX },
	{ "prefix", K_ESCPREFIX }
};

#define SIZE(a) (sizeof(a) / sizeof(a[0]))

int main(int argc, char **argv)
{
	unsigned int ometa, nmeta;
	struct meta *mp;
	int c;
	int fd = 0;
	char *console = NULL;

	const char *short_opts = "C:hV";
	const struct option long_opts[] = {
		{ "console", required_argument, NULL, 'C' },
		{ "help",    no_argument,       NULL, 'h' },
		{ "version", no_argument,       NULL, 'V' },
		{ NULL,      0,                 NULL,  0  }
	};

	set_progname(argv[0]);
	setuplocale();

	while ((c = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
		switch (c) {
			case 'C':
				if (optarg == NULL || optarg[0] == '\0')
					usage(EX_USAGE);
				console = optarg;
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

	if (console && (fd = getfd(console)) < 0)
		kbd_error(EXIT_FAILURE, 0, _("Couldn't get a file descriptor referring to the console"));

	if (ioctl(fd, KDGKBMETA, &ometa)) {
		kbd_error(EXIT_FAILURE, errno, _("Error reading current setting. Maybe stdin is not a VT?: "
		                                 "ioctl KDGKBMETA"));
	}

	if (optind == argc) {
		report(ometa);
		exit(EXIT_SUCCESS);
	}

	nmeta = 0; /* make gcc happy */
	for (mp = metas; (unsigned)(mp - metas) < SIZE(metas); mp++) {
		if (!strcmp(argv[1], mp->name)) {
			nmeta = mp->val;
			goto fnd;
		}
	}
	fprintf(stderr, _("unrecognized argument: _%s_\n\n"), argv[1]);
	usage(EXIT_FAILURE);

fnd:
	printf(_("old state:    "));
	report(ometa);
	if (ioctl(fd, KDSKBMETA, nmeta)) {
		kbd_error(EXIT_FAILURE, errno, "ioctl KDSKBMETA");
	}
	printf(_("new state:    "));
	report(nmeta);

	return EXIT_SUCCESS;
}
