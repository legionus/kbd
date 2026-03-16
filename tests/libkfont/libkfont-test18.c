#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sysexits.h>

#include <kfont.h>
#include "kfontP.h"
#include "libcommon.h"

static void
store_u32le(unsigned char *dst, unsigned int value)
{
	dst[0] = (unsigned char)(value & 0xffU);
	dst[1] = (unsigned char)((value >> 8) & 0xffU);
	dst[2] = (unsigned char)((value >> 16) & 0xffU);
	dst[3] = (unsigned char)((value >> 24) & 0xffU);
}

static void
free_uclist(struct unicode_list *heads, unsigned int count)
{
	unsigned int i;

	/* Error paths may leave a partially populated unicode table behind. */
	if (!heads)
		return;

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

static void
expect_read_failure(struct kfont_context *ctx, unsigned char *psf, unsigned int size,
		unsigned int glyph_count, int expected_ret, const char *what)
{
	unsigned char *allbuf = psf;
	unsigned char *fontbuf = NULL;
	struct unicode_list *uclistheads = NULL;
	unsigned int allsz = size;
	unsigned int fontsz = 0;
	unsigned int fontwidth = 0;
	unsigned int fontheight = 0;
	unsigned int fontlen = 0;
	int ret;

	ret = kfont_read_psffont(ctx, NULL, &allbuf, &allsz, &fontbuf, &fontsz,
			&fontwidth, &fontheight, &fontlen, 0, &uclistheads);
	if (ret != expected_ret)
		kbd_error(EXIT_FAILURE, 0, "%s: expected %d, got %d", what, expected_ret, ret);

	/*
	 * Negative tests intentionally stop in the middle of parsing, so cleanup
	 * has to handle both untouched and partially built unicode entries.
	 */
	free_uclist(uclistheads, glyph_count);
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct kfont_context *ctx;
	unsigned char short_table_psf[sizeof(struct psf2_header) + 1 + 1];
	unsigned char trailing_garbage_psf[sizeof(struct psf2_header) + 1 + 2];
	unsigned char zero_length_psf[sizeof(struct psf2_header) + 1];
	unsigned char zero_charsize_psf[sizeof(struct psf2_header)];

	if (kfont_init("libkfont-test18", &ctx) != 0)
		kbd_error(EXIT_FAILURE, 0, "unable to create kfont context");

	kfont_set_logger(ctx, NULL);

	/*
	 * Case 1: Unicode table is truncated before the glyph separator.
	 * The parser should notice end-of-buffer while still decoding entries.
	 */
	memset(short_table_psf, 0, sizeof(short_table_psf));
	short_table_psf[0] = PSF2_MAGIC0;
	short_table_psf[1] = PSF2_MAGIC1;
	short_table_psf[2] = PSF2_MAGIC2;
	short_table_psf[3] = PSF2_MAGIC3;
	store_u32le(short_table_psf + 8, sizeof(struct psf2_header));
	store_u32le(short_table_psf + 12, PSF2_HAS_UNICODE_TABLE);
	store_u32le(short_table_psf + 16, 1);
	store_u32le(short_table_psf + 20, 1);
	store_u32le(short_table_psf + 24, 1);
	store_u32le(short_table_psf + 28, 8);
	short_table_psf[sizeof(struct psf2_header)] = 0xaa;
	short_table_psf[sizeof(struct psf2_header) + 1] = 0x41;

	expect_read_failure(ctx, short_table_psf, sizeof(short_table_psf), 1,
			-EX_DATAERR, "short unicode table");

	/*
	 * Case 2: Glyph entry terminates correctly, but there are extra bytes left
	 * after the last separator, which should trigger the trailing-garbage check.
	 */
	memset(trailing_garbage_psf, 0, sizeof(trailing_garbage_psf));
	trailing_garbage_psf[0] = PSF2_MAGIC0;
	trailing_garbage_psf[1] = PSF2_MAGIC1;
	trailing_garbage_psf[2] = PSF2_MAGIC2;
	trailing_garbage_psf[3] = PSF2_MAGIC3;
	store_u32le(trailing_garbage_psf + 8, sizeof(struct psf2_header));
	store_u32le(trailing_garbage_psf + 12, PSF2_HAS_UNICODE_TABLE);
	store_u32le(trailing_garbage_psf + 16, 1);
	store_u32le(trailing_garbage_psf + 20, 1);
	store_u32le(trailing_garbage_psf + 24, 1);
	store_u32le(trailing_garbage_psf + 28, 8);
	trailing_garbage_psf[sizeof(struct psf2_header)] = 0xaa;
	trailing_garbage_psf[sizeof(struct psf2_header) + 1] = PSF2_SEPARATOR;
	trailing_garbage_psf[sizeof(struct psf2_header) + 2] = 0x99;

	expect_read_failure(ctx, trailing_garbage_psf, sizeof(trailing_garbage_psf), 1,
			-EX_DATAERR, "trailing garbage");

	/*
	 * Case 3: Header says the font contains zero glyphs.
	 * That should fail before any font-height or Unicode-table processing.
	 */
	memset(zero_length_psf, 0, sizeof(zero_length_psf));
	zero_length_psf[0] = PSF2_MAGIC0;
	zero_length_psf[1] = PSF2_MAGIC1;
	zero_length_psf[2] = PSF2_MAGIC2;
	zero_length_psf[3] = PSF2_MAGIC3;
	store_u32le(zero_length_psf + 8, sizeof(struct psf2_header));
	store_u32le(zero_length_psf + 16, 0);
	store_u32le(zero_length_psf + 20, 1);
	store_u32le(zero_length_psf + 24, 1);
	store_u32le(zero_length_psf + 28, 8);

	expect_read_failure(ctx, zero_length_psf, sizeof(zero_length_psf), 0,
			-EX_DATAERR, "zero font length");

	/*
	 * Case 4: Header says each glyph is zero bytes long.
	 * That should be rejected before the parser trusts the bitmap area.
	 */
	memset(zero_charsize_psf, 0, sizeof(zero_charsize_psf));
	zero_charsize_psf[0] = PSF2_MAGIC0;
	zero_charsize_psf[1] = PSF2_MAGIC1;
	zero_charsize_psf[2] = PSF2_MAGIC2;
	zero_charsize_psf[3] = PSF2_MAGIC3;
	store_u32le(zero_charsize_psf + 8, sizeof(struct psf2_header));
	store_u32le(zero_charsize_psf + 16, 1);
	store_u32le(zero_charsize_psf + 20, 0);
	store_u32le(zero_charsize_psf + 24, 1);
	store_u32le(zero_charsize_psf + 28, 8);

	expect_read_failure(ctx, zero_charsize_psf, sizeof(zero_charsize_psf), 1,
			-EX_DATAERR, "zero character size");

	kfont_free(ctx);

	return EXIT_SUCCESS;
}
