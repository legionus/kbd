#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "version.h"
#include "kfont.h"

int main(int argc, char *argv[])
{
	set_progname(argv[0]);

	if (argc != 2) {
		fprintf(stderr, "usage: %s FILENAME.psf\n", progname);
		exit(1);
	}

	FILE *f = fopen(argv[1], "rb");
	if (!f) {
		perror("fopen");
		exit(1);
	}

	struct kfont font;

	if (!kfont_read_file(f, &font)) {
		fprintf(stderr, "kfont_read_file failed\n");
		exit(1);
	}

	enum kfont_error error;
	error = kfont_parse_psf_font(&font);
	if (error != KFONT_ERROR_SUCCESS) {
		fprintf(stderr, "kfont_parse_psf_font: %d\n", error);
		exit(1);
	}

	kfont_free(&font);
}
