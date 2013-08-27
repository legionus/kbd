#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <linux/kd.h>
#include <errno.h>
#include <error.h>
#include "kbd.h"
#include "getfd.h"
#include "nls.h"
#include "version.h"

static unsigned char *cmap;

/* Standard VGA terminal colors, matching those hardcoded in the Linux kernel's
 * drivers/char/vt.c
 */
unsigned char vga_colors[] = {
	0x00, 0x00, 0x00,
	0xaa, 0x00, 0x00,
	0x00, 0xaa, 0x00,
	0xaa, 0x55, 0x00,
	0x00, 0x00, 0xaa,
	0xaa, 0x00, 0xaa,
	0x00, 0xaa, 0xaa,
	0xaa, 0xaa, 0xaa,
	0x55, 0x55, 0x55,
	0xff, 0x55, 0x55,
	0x55, 0xff, 0x55,
	0xff, 0xff, 0x55,
	0x55, 0x55, 0xff,
	0xff, 0x55, 0xff,
	0x55, 0xff, 0xff,
	0xff, 0xff, 0xff,
};

static void __attribute__ ((noreturn))
usage(int code)
{
	fprintf(stderr,
		_("Usage: %s vga|FILE|-\n"
		"\n"
		"If you use the FILE parameter, FILE should be exactly 3 lines of\n"
		"comma-separated decimal values for RED, GREEN, and BLUE.\n"
		"\n"
		"To seed a valid FILE:\n"
		"   cat /sys/module/vt/parameters/default_{red,grn,blu} > FILE\n"
		"\n"
		"and then edit the values in FILE.\n"
		"\n"),
		progname);
	exit(code);
}

static void
set_colormap(unsigned char *colormap)
{
	int fd = getfd(NULL);

	/* Apply the color map to the tty via ioctl */
	if (ioctl(fd, PIO_CMAP, colormap) == -1)
		error(EXIT_FAILURE, errno, "ioctl");

	close(fd);
}

static void
parse_file(FILE *fd, const char *filename)
{
	int c;
	unsigned int rows, cols, val;

	if ((cmap = calloc(3 * 16, sizeof(unsigned char))) == NULL)
		error(EXIT_FAILURE, errno, "calloc");

	for (rows = 0; rows < 3; rows++) {
		cols = 0;

		while (cols < 16) {
			if ((c = fscanf(fd, "%u", &val)) != 1) {
				if (c == EOF)
					error(EXIT_FAILURE, errno, "fscanf");

				error(EXIT_FAILURE, 0, _("Error: %s: Invalid value in field %u in line %u."),
				      filename, rows + 1, cols + 1);
			}

			cmap[rows + cols * 3] = (unsigned char) val;

			if (cols < 15 && fgetc(fd) != ',')
				error(EXIT_FAILURE, 0, _("Error: %s: Insufficient number of fields in line %u."),
				      filename, rows + 1);
			cols++;
		}

		if ((c = fgetc(fd)) == EOF)
			error(EXIT_FAILURE, 0, _("Error: %s: Line %u has ended unexpectedly.\n"),
			      filename, rows + 1); 

		if (c != '\n')
			error(EXIT_FAILURE, 0, _("Error: %s: Line %u is too long.\n"),
			      filename, rows + 1);
	}
}

int
main(int argc, char **argv) {
	int c;
	const char *file;
	FILE *fd;

	set_progname(argv[0]);

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE_NAME, LOCALEDIR);
	textdomain(PACKAGE_NAME);

	while ((c = getopt(argc, argv, "hV")) != EOF) {
		switch (c) {
			case 'V':
				print_version_and_exit();
				break;
			case 'h':
				usage(EXIT_SUCCESS);
				break;
		}
	}

	if (optind == argc)
		usage(EXIT_FAILURE);

	file = argv[optind];

	if (!strcmp(file, "vga")) {
		set_colormap(vga_colors);
		return EXIT_SUCCESS;

	} else if (!strcmp(file, "-")) {
		parse_file(stdin, "stdin");

	} else {
		if ((fd = fopen(file, "r")) == NULL)
			error(EXIT_FAILURE, errno, "fopen");

		parse_file(fd, file);
		fclose(fd);
	}

	set_colormap(cmap);
	free(cmap);

	return EXIT_SUCCESS;
}
