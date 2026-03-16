#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <sysexits.h>

#include <kfont.h>
#include "kfontP.h"
#include "libcommon.h"

#define TEST_GLYPH_COUNT 2U
#define TEST_CHARSIZE 2U

static void
store_u32le(unsigned char *dst, unsigned int value)
{
	/* PSF2 stores all scalar header fields in little-endian form. */
	dst[0] = (unsigned char)(value & 0xffU);
	dst[1] = (unsigned char)((value >> 8) & 0xffU);
	dst[2] = (unsigned char)((value >> 16) & 0xffU);
	dst[3] = (unsigned char)((value >> 24) & 0xffU);
}

static void
free_uclist(struct unicode_list *heads, unsigned int count)
{
	unsigned int i;

	for (i = 0; i < count; i++) {
		struct unicode_list *ul = heads[i].next;

		while (ul) {
			struct unicode_list *next_ul = ul->next;
			struct unicode_seq *us = ul->seq;

			while (us) {
				struct unicode_seq *next_us = us->next;

				free(us);
				us = next_us;
			}

			free(ul);
			ul = next_ul;
		}
	}

	free(heads);
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct kfont_context *ctx;
	unsigned char psf[sizeof(struct psf2_header) + TEST_GLYPH_COUNT * TEST_CHARSIZE + 9];
	unsigned char *allbuf = NULL;
	unsigned char *fontbuf = NULL;
	struct unicode_list *uclistheads = NULL;
	unsigned int allsz = 0;
	unsigned int fontsz = 0;
	unsigned int fontwidth = 0;
	unsigned int fontheight = 0;
	unsigned int fontlen = 0;
	FILE *fp;
	struct unicode_list *ul;
	struct unicode_seq *us;

	if (kfont_init("libkfont-test15", &ctx) != 0)
		kbd_error(EXIT_FAILURE, 0, "unable to create kfont context");

	kfont_set_logger(ctx, NULL);

	/*
	 * Build a minimal PSF2 image directly in memory:
	 * - 2 glyphs
	 * - 2 bytes per glyph bitmap
	 * - explicit Unicode table
	 */
	memset(psf, 0, sizeof(psf));
	psf[0] = PSF2_MAGIC0;
	psf[1] = PSF2_MAGIC1;
	psf[2] = PSF2_MAGIC2;
	psf[3] = PSF2_MAGIC3;
	/* version stays 0 because that is the only version the parser accepts. */
	store_u32le(psf + 8, sizeof(struct psf2_header));
	store_u32le(psf + 12, PSF2_HAS_UNICODE_TABLE);
	store_u32le(psf + 16, TEST_GLYPH_COUNT);
	store_u32le(psf + 20, TEST_CHARSIZE);
	/* In the PSF2 header the order is height, then width. */
	store_u32le(psf + 24, 2);
	store_u32le(psf + 28, 8);

	/* Glyph payload itself is arbitrary; we only need stable bytes to verify offsets. */
	psf[sizeof(struct psf2_header) + 0] = 0x11;
	psf[sizeof(struct psf2_header) + 1] = 0x22;
	psf[sizeof(struct psf2_header) + 2] = 0x33;
	psf[sizeof(struct psf2_header) + 3] = 0x44;

	/*
	 * Unicode table:
	 * - glyph 0 => U+0041 ('A')
	 * - glyph 1 => U+00E9 ('é')
	 * - then a sequence for glyph 1: U+0065 U+0301 ('e' + combining acute)
	 */
	psf[sizeof(struct psf2_header) + 4] = 0x41;
	psf[sizeof(struct psf2_header) + 5] = PSF2_SEPARATOR;
	psf[sizeof(struct psf2_header) + 6] = 0xc3;
	psf[sizeof(struct psf2_header) + 7] = 0xa9;
	psf[sizeof(struct psf2_header) + 8] = PSF2_STARTSEQ;
	psf[sizeof(struct psf2_header) + 9] = 0x65;
	psf[sizeof(struct psf2_header) + 10] = 0xcc;
	psf[sizeof(struct psf2_header) + 11] = 0x81;
	psf[sizeof(struct psf2_header) + 12] = PSF2_SEPARATOR;

	fp = tmpfile();
	if (!fp)
		kbd_error(EXIT_FAILURE, errno, "tmpfile failed");

	if (fwrite(psf, sizeof(psf), 1, fp) != 1)
		kbd_error(EXIT_FAILURE, errno, "unable to write psf2 test image");

	rewind(fp);

	if (kfont_read_psffont(ctx, fp, &allbuf, &allsz, &fontbuf, &fontsz,
			&fontwidth, &fontheight, &fontlen, 0, &uclistheads) != 0)
		kbd_error(EXIT_FAILURE, 0, "kfont_read_psffont failed");

	fclose(fp);

	if (allsz != sizeof(psf))
		kbd_error(EXIT_FAILURE, 0, "unexpected full buffer size: %u", allsz);

	if (fontbuf != allbuf + sizeof(struct psf2_header))
		kbd_error(EXIT_FAILURE, 0, "unexpected font buffer offset");

	if (fontsz != TEST_GLYPH_COUNT * TEST_CHARSIZE)
		kbd_error(EXIT_FAILURE, 0, "unexpected font size: %u", fontsz);

	if (fontwidth != 8U || fontheight != 2U || fontlen != TEST_GLYPH_COUNT)
		kbd_error(EXIT_FAILURE, 0, "unexpected font geometry: %ux%u len=%u",
				fontwidth, fontheight, fontlen);

	/* Glyph 0 should decode into a single mapping: A. */
	ul = uclistheads[0].next;
	if (!ul || ul->seq->uc != 0x0041 || ul->seq->next != NULL || ul->next != NULL)
		kbd_error(EXIT_FAILURE, 0, "unexpected first unicode entry");

	/* Glyph 1 should first contain the precomposed character é. */
	ul = uclistheads[1].next;
	if (!ul || ul->seq->uc != 0x00e9)
		kbd_error(EXIT_FAILURE, 0, "unexpected second glyph primary unicode");

	/* The next entry for glyph 1 is the decomposed sequence e + combining acute. */
	ul = ul->next;
	if (!ul)
		kbd_error(EXIT_FAILURE, 0, "missing second glyph sequence entry");

	us = ul->seq;
	if (!us || us->uc != 0x0065)
		kbd_error(EXIT_FAILURE, 0, "unexpected sequence base unicode");

	us = us->next;
	if (!us || us->uc != 0x0301 || us->next != NULL)
		kbd_error(EXIT_FAILURE, 0, "unexpected sequence continuation");

	free_uclist(uclistheads, TEST_GLYPH_COUNT);
	free(allbuf);
	kfont_free(ctx);

	return EXIT_SUCCESS;
}
