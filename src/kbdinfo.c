#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/kd.h>
#include <getopt.h>
#include <sysexits.h>

#include "libcommon.h"

static const char *action = NULL;
static const char *value  = NULL;

static void __attribute__((noreturn))
usage(int code)
{
	fprintf(stderr,
	        _("Usage: %1$s [options] getmode [text|graphics]\n"
	          "   or: %1$s [options] gkbmode [raw|xlate|mediumraw|unicode]\n"
	          "   or: %1$s [options] gkbmeta [metabit|escprefix]\n"
	          "   or: %1$s [options] gkbled  [scrolllock|numlock|capslock]\n"
	          "\n"
	          "The utility allows to read and check various parameters\n"
	          "of the keyboard and virtual console.\n"
	          "\n"
	          "Options:\n"
	          "  -C, --console=DEV     the console device to be used;\n"
	          "  -h, --help            print this usage message;\n"
	          "  -V, --version         print version number.\n"
	         ),
	        get_progname());
	exit(code);
}

static int
answer(const char *ans)
{
	if (value)
		return strcasecmp(value, ans) ? EXIT_FAILURE : EXIT_SUCCESS;

	printf("%s\n", ans);
	return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
	int fd, mode, c;
	int rc = EXIT_FAILURE;
	char flags;
	const char *console = NULL;

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

	if (optind == argc) {
		kbd_error(EXIT_FAILURE, 0, _("Error: Not enough arguments.\n"));
	}

	action = argv[optind++];

	if (optind < argc)
		value = argv[optind++];

	if ((fd = getfd(console)) < 0)
		kbd_error(EXIT_FAILURE, 0, _("Couldn't get a file descriptor referring to the console"));

	if (!strcasecmp("GETMODE", action)) {
		if (ioctl(fd, KDGETMODE, &mode) == -1)
			kbd_error(EXIT_FAILURE, errno, "ioctl KDGETMODE");

		switch (mode) {
			case KD_TEXT:
				rc = answer("text");
				break;
			case KD_GRAPHICS:
				rc = answer("graphics");
				break;
		}

	} else if (!strcasecmp("GKBMODE", action)) {
		if (ioctl(fd, KDGKBMODE, &mode) == -1)
			kbd_error(EXIT_FAILURE, errno, "ioctl KDGKBMODE");

		switch (mode) {
			case K_RAW:
				rc = answer("raw");
				break;
			case K_XLATE:
				rc = answer("xlate");
				break;
			case K_MEDIUMRAW:
				rc = answer("mediumraw");
				break;
			case K_UNICODE:
				rc = answer("unicode");
				break;
		}

	} else if (!strcasecmp("GKBMETA", action)) {
		if (ioctl(fd, KDGKBMETA, &mode) == -1)
			kbd_error(EXIT_FAILURE, errno, "ioctl KDGKBMETA");

		switch (mode) {
			case K_METABIT:
				rc = answer("metabit");
				break;
			case K_ESCPREFIX:
				rc = answer("escprefix");
				break;
		}

	} else if (!strcasecmp("GKBLED", action)) {
		if (ioctl(fd, KDGKBLED, &flags) == -1)
			kbd_error(EXIT_FAILURE, errno, "ioctl KDGKBLED");

		mode = (flags & 0x7);

		if (value) {
			if (((mode & LED_SCR) && !strcasecmp(value, "scrolllock")) ||
			    ((mode & LED_NUM) && !strcasecmp(value, "numlock")) ||
			    ((mode & LED_CAP) && !strcasecmp(value, "capslock")))
				rc = EXIT_SUCCESS;
		} else {
			printf("scrolllock:%s ", (mode & LED_SCR) ? "on" : "off");
			printf("numlock:%s ", (mode & LED_NUM) ? "on" : "off");
			printf("capslock:%s\n", (mode & LED_CAP) ? "on" : "off");
			rc = EXIT_SUCCESS;
		}

	} else {
		kbd_warning(0, _("Error: Unrecognized action: %s\n"), action);
	}

	close(fd);
	return rc;
}
