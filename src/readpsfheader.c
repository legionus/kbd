#include "config.h"

#include <sysexits.h>
#include <stdio.h>
#include <stdlib.h> /* exit */

#include <kfont.h>

#include "libcommon.h"

static void KBD_ATTR_NORETURN
usage(void)
{
	fprintf(stderr, "usage: readpsfheader font.psf\n");
	exit(EX_USAGE);
}

int main(int argc, char **argv)
{
	int psftype, fontlen, charsize, hastable, notable;
	int width = 8, bytewidth, height;
	char *inbuf, *fontbuf;
	int inbuflth, fontbuflth;
	struct unicode_list *uclistheads = NULL;

	setuplocale();

	if (argc != 2)
		usage();

	FILE *f = fopen(argv[1], "rb");
	if (!f) {
		perror("fopen");
		return EX_NOINPUT;
	}

	int ret;
	struct kfont_context *kfont;

	if ((ret = kfont_init(program_invocation_short_name, &kfont)) < 0)
		return -ret;

	if (kfont_read_psffont(kfont, f, &inbuf, &inbuflth, &fontbuf, &fontbuflth, &width, &fontlen, 0, &uclistheads) < 0)
		kbd_error(EX_DATAERR, 0, "Bad magic number");

	close(f);

	return EX_OK;
}
