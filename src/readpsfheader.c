#include "config.h"

#include <stdio.h>
#include <stdlib.h> /* exit */

#include "libcommon.h"
#include "kfont.h"

static void __attribute__((noreturn))
usage(void)
{
	fprintf(stderr, "usage: readpsfheader font.psf\n");
	exit(1);
}

int main(int argc, char **argv)
{
	int psftype, fontlen, charsize, hastable, notable;
	int width = 8, bytewidth, height;
	char *inbuf, *fontbuf;
	int inbuflth, fontbuflth;
	struct unicode_list *uclistheads;

	set_progname(argv[0]);
	setuplocale();

	if (argc != 2)
		usage();

	FILE *f = fopen(argv[1], "rb");
	if (!f) {
		perror("fopen");
		exit(1);
	}

	struct kfont_context ctx;
	kfont_init(&ctx);

	if (readpsffont(&ctx, f, &inbuf, &inbuflth, &fontbuf, &fontbuflth, &width, &fontlen, 0, &uclistheads) < 0) {
		fprintf(stderr, "%s: Bad magic number\n", argv[0]);
		return 1;
	}
	close(f);

	return 0;
}
