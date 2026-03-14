#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <kfont.h>
#include "libcommon.h"

#define TEST_GLYPH_COUNT 256U
#define TEST_GLYPH_SIZE 2U

static void
fill_test_font(unsigned char *buf, size_t len)
{
	size_t i;

	for (i = 0; i < len; i++)
		buf[i] = (unsigned char)((i * 5U) & 0xffU);
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct kfont_context *ctx;
	unsigned char psf[sizeof(struct psf1_header) + TEST_GLYPH_COUNT * TEST_GLYPH_SIZE];
	unsigned char *allbuf = NULL;
	unsigned char *fontbuf = NULL;
	unsigned int allsz = 0;
	unsigned int fontsz = 0;
	unsigned int fontwidth = 0;
	unsigned int fontheight = 0;
	unsigned int fontlen = 0;
	FILE *fp;

	if (kfont_init("libkfont-test13", &ctx) != 0)
		kbd_error(EXIT_FAILURE, 0, "unable to create kfont context");

	kfont_set_logger(ctx, NULL);

	memset(psf, 0, sizeof(psf));
	psf[0] = PSF1_MAGIC0;
	psf[1] = PSF1_MAGIC1;
	psf[2] = 0;
	psf[3] = TEST_GLYPH_SIZE;
	fill_test_font(psf + sizeof(struct psf1_header),
		sizeof(psf) - sizeof(struct psf1_header));

	fp = tmpfile();
	if (!fp)
		kbd_error(EXIT_FAILURE, 0, "tmpfile failed");

	if (fwrite(psf, sizeof(psf), 1, fp) != 1)
		kbd_error(EXIT_FAILURE, 0, "unable to write psf test image");

	rewind(fp);

	if (kfont_read_psffont(ctx, fp, &allbuf, &allsz, &fontbuf, &fontsz,
			&fontwidth, &fontheight, &fontlen, 0, NULL) != 0)
		kbd_error(EXIT_FAILURE, 0, "kfont_read_psffont failed");

	fclose(fp);

	if (allsz != sizeof(psf))
		kbd_error(EXIT_FAILURE, 0, "unexpected full buffer size: %u", allsz);

	if (fontbuf != allbuf + sizeof(struct psf1_header))
		kbd_error(EXIT_FAILURE, 0, "unexpected font buffer offset");

	if (fontsz != TEST_GLYPH_COUNT * TEST_GLYPH_SIZE)
		kbd_error(EXIT_FAILURE, 0, "unexpected font size: %u", fontsz);

	if (fontwidth != 8U)
		kbd_error(EXIT_FAILURE, 0, "unexpected font width: %u", fontwidth);

	if (fontheight != TEST_GLYPH_SIZE)
		kbd_error(EXIT_FAILURE, 0, "unexpected font height: %u", fontheight);

	if (fontlen != TEST_GLYPH_COUNT)
		kbd_error(EXIT_FAILURE, 0, "unexpected font length: %u", fontlen);

	if (memcmp(fontbuf, psf + sizeof(struct psf1_header), fontsz) != 0)
		kbd_error(EXIT_FAILURE, 0, "unexpected glyph data");

	free(allbuf);
	kfont_free(ctx);

	return EXIT_SUCCESS;
}
