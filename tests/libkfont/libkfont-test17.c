#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
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
read_back(FILE *fp, unsigned char *buf, size_t size)
{
	size_t nread;

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
	return ((unsigned int)src[0]) |
		((unsigned int)src[1] << 8) |
		((unsigned int)src[2] << 16) |
		((unsigned int)src[3] << 24);
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct kfont_context *ctx;
	unsigned char bad_psf[sizeof(struct psf2_header) + 1 + 2];
	unsigned char header_out[sizeof(struct psf2_header)];
	unsigned char unicode_out[3];
	unsigned char *allbuf;
	unsigned char *fontbuf = NULL;
	struct unicode_list *uclistheads = NULL;
	unsigned int allsz;
	unsigned int fontsz = 0;
	unsigned int fontwidth = 0;
	unsigned int fontheight = 0;
	unsigned int fontlen = 0;
	FILE *fp;
	int psftype;
	int ret;

	if (kfont_init("libkfont-test17", &ctx) != 0)
		kbd_error(EXIT_FAILURE, 0, "unable to create kfont context");

	kfont_set_logger(ctx, NULL);

	/*
	 * Malformed PSF2 image:
	 * - one glyph, one bitmap byte
	 * - Unicode table starts with 0xC3, which is an incomplete UTF-8 sequence
	 * - followed by a separator so the buffer is long enough, but still invalid
	 */
	memset(bad_psf, 0, sizeof(bad_psf));
	bad_psf[0] = PSF2_MAGIC0;
	bad_psf[1] = PSF2_MAGIC1;
	bad_psf[2] = PSF2_MAGIC2;
	bad_psf[3] = PSF2_MAGIC3;
	store_u32le(bad_psf + 8, sizeof(struct psf2_header));
	store_u32le(bad_psf + 12, PSF2_HAS_UNICODE_TABLE);
	store_u32le(bad_psf + 16, 1);
	store_u32le(bad_psf + 20, 1);
	store_u32le(bad_psf + 24, 1);
	store_u32le(bad_psf + 28, 8);
	bad_psf[sizeof(struct psf2_header)] = 0xaa;
	bad_psf[sizeof(struct psf2_header) + 1] = 0xc3;
	bad_psf[sizeof(struct psf2_header) + 2] = PSF2_SEPARATOR;

	allbuf = bad_psf;
	allsz = sizeof(bad_psf);
	ret = kfont_read_psffont(ctx, NULL, &allbuf, &allsz, &fontbuf, &fontsz,
			&fontwidth, &fontheight, &fontlen, 0, &uclistheads);
	if (ret != -EX_DATAERR)
		kbd_error(EXIT_FAILURE, 0, "expected invalid UTF-8 failure, got %d", ret);

	/*
	 * Even on parse failure the reader may already have allocated the glyph
	 * table head array, so the test must release it explicitly.
	 */
	free(uclistheads);

	/*
	 * writepsffontheader() should silently upgrade the output to PSF2 when
	 * geometry cannot be represented as PSF1. Width 9 is enough to force that.
	 */
	fp = tmpfile();
	if (!fp)
		kbd_error(EXIT_FAILURE, errno, "tmpfile failed");

	psftype = 1;
	ret = writepsffontheader(ctx, fp, 9, 3, 3, &psftype, WPSFH_HASTAB);
	if (ret != 0 || psftype != 2)
		kbd_error(EXIT_FAILURE, 0, "unexpected writepsffontheader result");

	read_back(fp, header_out, sizeof(header_out));
	fclose(fp);

	if (header_out[0] != PSF2_MAGIC0 || header_out[1] != PSF2_MAGIC1 ||
			header_out[2] != PSF2_MAGIC2 || header_out[3] != PSF2_MAGIC3)
		kbd_error(EXIT_FAILURE, 0, "unexpected helper-written PSF2 magic");

	if (read_u32le(header_out + 8) != sizeof(struct psf2_header) ||
			read_u32le(header_out + 12) != PSF2_HAS_UNICODE_TABLE ||
			read_u32le(header_out + 16) != 3 ||
			read_u32le(header_out + 20) != 6 ||
			read_u32le(header_out + 24) != 3 ||
			read_u32le(header_out + 28) != 9)
		kbd_error(EXIT_FAILURE, 0, "unexpected helper-written PSF2 header fields");

	/*
	 * appendunicode() should emit raw UTF-8 bytes for PSF2 and reject
	 * negative codepoints before any write happens.
	 */
	fp = tmpfile();
	if (!fp)
		kbd_error(EXIT_FAILURE, errno, "tmpfile failed");

	ret = appendunicode(ctx, fp, 0x00e9, 1);
	if (ret != 0)
		kbd_error(EXIT_FAILURE, 0, "appendunicode failed for UTF-8 path");

	ret = appendseparator(ctx, fp, 0, 1);
	if (ret != 0)
		kbd_error(EXIT_FAILURE, 0, "appendseparator failed");

	read_back(fp, unicode_out, sizeof(unicode_out));
	fclose(fp);

	if (memcmp(unicode_out, "\xc3\xa9\xff", 3) != 0)
		kbd_error(EXIT_FAILURE, 0, "unexpected UTF-8 unicode encoding");

	ret = appendunicode(ctx, stdout, -1, 1);
	if (ret != -EX_DATAERR)
		kbd_error(EXIT_FAILURE, 0, "expected appendunicode to reject negative unicode");

	kfont_free(ctx);

	return EXIT_SUCCESS;
}
