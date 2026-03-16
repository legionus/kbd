#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <sysexits.h>

#include <kfont.h>
#include "kfontP.h"
#include "libcommon.h"

static void
read_back(FILE *fp, unsigned char *buf, size_t size)
{
	size_t nread;

	/* Flush and rewind so we can assert on the exact bytes written by the helpers. */
	if (fflush(fp) != 0)
		kbd_error(EXIT_FAILURE, errno, "fflush failed");

	if (fseek(fp, 0, SEEK_SET) != 0)
		kbd_error(EXIT_FAILURE, errno, "fseek failed");

	nread = fread(buf, 1, size, fp);
	if (nread != size)
		kbd_error(EXIT_FAILURE, 0, "fread read %zu of %zu bytes", nread, size);
}

static unsigned int
read_u32le(const unsigned char *src)
{
	/* PSF2 headers are little-endian regardless of host endianness. */
	return ((unsigned int)src[0]) |
		((unsigned int)src[1] << 8) |
		((unsigned int)src[2] << 16) |
		((unsigned int)src[3] << 24);
}

static void
free_uclist(struct unicode_list *head)
{
	struct unicode_list *ul = head->next;

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

	clear_uni_entry(head);
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct kfont_context *ctx;
	unsigned char psf1_font[256 * 2];
	unsigned char psf1_out[sizeof(struct psf1_header) + sizeof(psf1_font)];
	unsigned char psf2_font[3 * 4];
	unsigned char psf2_out[sizeof(struct psf2_header) + sizeof(psf2_font) + 10];
	struct unicode_list uclistheads[3];
	FILE *fp;
	unsigned int i;
	int ret;
	int psftype;

	if (kfont_init("libkfont-test16", &ctx) != 0)
		kbd_error(EXIT_FAILURE, 0, "unable to create kfont context");

	kfont_set_logger(ctx, NULL);

	/* First check the simple PSF1 path: 8-pixel-wide, 256 glyphs, no Unicode table. */
	for (i = 0; i < sizeof(psf1_font); i++)
		psf1_font[i] = (unsigned char)((i * 3U) & 0xffU);

	fp = tmpfile();
	if (!fp)
		kbd_error(EXIT_FAILURE, errno, "tmpfile failed");

	ret = kfont_write_psffont(ctx, fp, psf1_font, 8, 2, 256, 1, NULL);
	if (ret != 0)
		kbd_error(EXIT_FAILURE, 0, "unexpected PSF1 write result: %d", ret);

	read_back(fp, psf1_out, sizeof(psf1_out));
	fclose(fp);

	if (psf1_out[0] != PSF1_MAGIC0 || psf1_out[1] != PSF1_MAGIC1 ||
			psf1_out[2] != 0 || psf1_out[3] != 2)
		kbd_error(EXIT_FAILURE, 0, "unexpected PSF1 header");

	if (memcmp(psf1_out + sizeof(struct psf1_header), psf1_font, sizeof(psf1_font)) != 0)
		kbd_error(EXIT_FAILURE, 0, "unexpected PSF1 glyph payload");

	/*
	 * Prepare Unicode entries for a PSF2 write:
	 * - glyph 0 => U+0041
	 * - glyph 1 => sequence U+0065 U+0301
	 * - glyph 2 => U+00E9
	 *
	 * addpair() starts a new Unicode entry for the glyph, while addseq()
	 * appends another codepoint to the most recently added entry.
	 */
	for (i = 0; i < 3; i++)
		clear_uni_entry(&uclistheads[i]);

	if (addpair(&uclistheads[0], 0x0041) != 0)
		kbd_error(EXIT_FAILURE, 0, "addpair failed");

	if (addpair(&uclistheads[1], 0x0065) != 0)
		kbd_error(EXIT_FAILURE, 0, "addpair failed");

	if (addseq(&uclistheads[1], 0x0301) != 0)
		kbd_error(EXIT_FAILURE, 0, "addseq failed");

	if (addpair(&uclistheads[2], 0x00e9) != 0)
		kbd_error(EXIT_FAILURE, 0, "addpair failed");

	/* Non-PSF1 geometry forces kfont_write_psffont() to upgrade the output to PSF2. */
	for (i = 0; i < sizeof(psf2_font); i++)
		psf2_font[i] = (unsigned char)(0xa0U + i);

	fp = tmpfile();
	if (!fp)
		kbd_error(EXIT_FAILURE, errno, "tmpfile failed");

	psftype = 1;
	ret = kfont_write_psffont(ctx, fp, psf2_font, 9, 2, 3, psftype, uclistheads);
	if (ret != 1)
		kbd_error(EXIT_FAILURE, 0, "unexpected PSF2 write result: %d", ret);

	read_back(fp, psf2_out, sizeof(psf2_out));
	fclose(fp);

	if (psf2_out[0] != PSF2_MAGIC0 || psf2_out[1] != PSF2_MAGIC1 ||
			psf2_out[2] != PSF2_MAGIC2 || psf2_out[3] != PSF2_MAGIC3)
		kbd_error(EXIT_FAILURE, 0, "unexpected PSF2 magic");

	if (read_u32le(psf2_out + 4) != 0 ||
			read_u32le(psf2_out + 8) != sizeof(struct psf2_header) ||
			read_u32le(psf2_out + 12) != PSF2_HAS_UNICODE_TABLE ||
			read_u32le(psf2_out + 16) != 3 ||
			read_u32le(psf2_out + 20) != 4 ||
			read_u32le(psf2_out + 24) != 2 ||
			read_u32le(psf2_out + 28) != 9)
		kbd_error(EXIT_FAILURE, 0, "unexpected PSF2 header fields");

	if (memcmp(psf2_out + sizeof(struct psf2_header), psf2_font, sizeof(psf2_font)) != 0)
		kbd_error(EXIT_FAILURE, 0, "unexpected PSF2 glyph payload");

	/*
	 * Unicode payload layout:
	 * - glyph 0: "A" + separator
	 * - glyph 1: STARTSEQ + "e" + combining acute + separator
	 * - glyph 2: "é" + separator
	 */
	if (memcmp(psf2_out + sizeof(struct psf2_header) + sizeof(psf2_font),
			"\x41\xff\xfe\x65\xcc\x81\xff\xc3\xa9\xff", 10) != 0)
		kbd_error(EXIT_FAILURE, 0, "unexpected PSF2 unicode table");

	free_uclist(&uclistheads[0]);
	free_uclist(&uclistheads[1]);
	free_uclist(&uclistheads[2]);
	kfont_free(ctx);

	return EXIT_SUCCESS;
}
