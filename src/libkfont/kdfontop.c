/*
 * kdfontop.c - export getfont(), getfontsize() and putfont()
 *
 * Font handling differs between various kernel versions.
 * Hide the differences in this file.
 */
#include "config.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h> /* free() */
#include <string.h>
#include <sys/ioctl.h>
#include <linux/kd.h>

#include "libcommon.h"
#include "contextP.h"

#ifdef COMPAT_HEADERS
#include "compat/linux-kd.h"
#endif

int
kfont_restore_font(struct kfont_ctx *ctx)
{
	char errbuf[STACKBUF_LEN];

	if (ctx->consolefd < 0) {
		ERR(ctx, "console descriptor must be specified");
		return -1;
	}

	if (ioctl(ctx->consolefd, PIO_FONTRESET, 0)) {
		strerror_r(errno, errbuf, sizeof(errbuf));
		ERR(ctx, "kfont_restore_font: ioctl(PIO_FONTRESET): %s", errbuf);
		return -1;
	}
	return 0;
}

size_t
kfont_get_charheight(unsigned char *buf, size_t count, size_t width)
{
	size_t h, i, x;
	size_t bytewidth = (width + 7) / 8;

	for (h = 32; h > 0; h--)
		for (i = 0; i < count; i++)
			for (x = 0; x < bytewidth; x++)
				if (buf[(32 * i + h - 1) * bytewidth + x])
					goto nonzero;
nonzero:
	return h;
}

/*
 * May be called with buf==NULL if we only want info.
 * May be called with width==NULL and height==NULL.
 * Must not exit - we may have cleanup to do.
 */
int
kfont_get_font(struct kfont_ctx *ctx, unsigned char *buf, size_t *count, size_t *width, size_t *height)
{
	char errbuf[STACKBUF_LEN];

	if (count == NULL) {
		ERR(ctx, "bug: getfont called with count == NULL");
		return -1;
	}

	if (*count > USHRT_MAX) {
		ERR(ctx, "bug: getfont called with count >= %d", USHRT_MAX);
		return -1;
	}

	if (ctx->consolefd < 0) {
		ERR(ctx, "console descriptor must be specified");
		return -1;
	}

	/* First attempt: KDFONTOP */
	struct console_font_op cfo;

	cfo.op = KD_FONT_OP_GET;
	cfo.flags = 0;
	cfo.width = 32;
	cfo.height = 32;
	cfo.charcount = (unsigned short) *count;
	cfo.data = buf;

	if (ioctl(ctx->consolefd, KDFONTOP, &cfo) == 0) {
		*count = cfo.charcount;
		if (height)
			*height = cfo.height;
		if (width)
			*width = cfo.width;
		return 0;
	}
	if (errno != ENOSYS && errno != EINVAL) {
		strerror_r(errno, errbuf, sizeof(errbuf));
		ERR(ctx, "getfont: ioctl(KDFONTOP): %s", errbuf);
		return -1;
	}

	/* The other methods do not support width != 8 */
	if (width)
		*width = 8;

	/* Second attempt: GIO_FONTX */
	struct consolefontdesc cfd;

	cfd.charcount = (unsigned short) *count;
	cfd.charheight = 0;
	cfd.chardata = (char *) buf;

	if (ioctl(ctx->consolefd, GIO_FONTX, &cfd) == 0) {
		*count = cfd.charcount;
		if (height)
			*height = cfd.charheight;
		return 0;
	}
	if (errno != ENOSYS && errno != EINVAL) {
		strerror_r(errno, errbuf, sizeof(errbuf));
		ERR(ctx, "getfont: ioctl(GIO_FONTX): %s", errbuf);
		return -1;
	}

	/* Third attempt: GIO_FONT */
	if (*count < 256) {
		ERR(ctx, _("bug: getfont called with count<256"));
		return -1;
	}

	if (!buf) {
		ERR(ctx, _("bug: getfont using GIO_FONT needs buf"));
		return -1;
	}

	if (ioctl(ctx->consolefd, GIO_FONT, buf)) {
		strerror_r(errno, errbuf, sizeof(errbuf));
		ERR(ctx, "getfont: ioctl(GIO_FONT): %s", errbuf);
		return -1;
	}

	*count = 256;

	if (height)
		*height = 0; /* undefined, at most 32 */

	return 0;
}

size_t
kfont_get_fontsize(struct kfont_ctx *ctx)
{
	size_t count = 0;
	return (kfont_get_font(ctx, NULL, &count, NULL, NULL) == 0) ? count : 256;
}

int
kfont_load_font(struct kfont_ctx *ctx, unsigned char *buf, size_t count, size_t width, size_t height)
{
	char errbuf[STACKBUF_LEN];

	if (count > USHRT_MAX) {
		ERR(ctx, "bug: putfont called with count > %d", USHRT_MAX);
		return -1;
	}

	if (width > UINT_MAX) {
		ERR(ctx, "bug: putfont called with width > %d", UINT_MAX);
		return -1;
	}

	if (height > UINT_MAX) {
		ERR(ctx, "bug: putfont called with height > %d", UINT_MAX);
		return -1;
	}

	if (ctx->consolefd < 0) {
		ERR(ctx, "console descriptor must be specified");
		return -1;
	}

	if (!width)
		width = 8;

	if (!height)
		height = kfont_get_charheight(buf, count, width);

	/* First attempt: KDFONTOP */
	struct console_font_op cfo;

	cfo.op = KD_FONT_OP_SET;
	cfo.flags = 0;
	cfo.width = (unsigned int) width;
	cfo.height = (unsigned int) height;
	cfo.charcount = (unsigned short) count;
	cfo.data = buf;

	if (ioctl(ctx->consolefd, KDFONTOP, &cfo) == 0)
		return 0;

	if (width != 8 || (errno != ENOSYS && errno != EINVAL)) {
		strerror_r(errno, errbuf, sizeof(errbuf));
		ERR(ctx, "putfont: ioctl(KDFONTOP): %s", errbuf);
		return -1;
	}

	/* Variation on first attempt: in case count is not 256 or 512
	   round up and try again. */
	if (errno == EINVAL && width == 8 && count != 256 && count < 512) {
		unsigned int ct = ((count > 256) ? 512 : 256);

		unsigned char *mybuf = malloc(32U * ct);
		if (!mybuf) {
			ERR(ctx, "out of memory");
			return -1;
		}

		memset(mybuf, 0, 32U * ct);
		memcpy(mybuf, buf, 32U * count);

		cfo.data = mybuf;
		cfo.charcount = ct;

		int i = ioctl(ctx->consolefd, KDFONTOP, &cfo);
		free(mybuf);

		if (i == 0)
			return 0;
	}

	/* Second attempt: PIO_FONTX */
	struct consolefontdesc cfd;

	cfd.charcount = (unsigned short) count;
	cfd.charheight = (unsigned short) height;
	cfd.chardata = (char *) buf;

	if (ioctl(ctx->consolefd, PIO_FONTX, &cfd) == 0)
		return 0;

	if (errno != ENOSYS && errno != EINVAL) {
		strerror_r(errno, errbuf, sizeof(errbuf));
		ERR(ctx, "putfont: ioctl(PIO_FONTX): %d,%dx%d: %s", count, width, height, errbuf);
		return -1;
	}

	/* Third attempt: PIO_FONT */
	/* This will load precisely 256 chars, independent of count */
	if (ioctl(ctx->consolefd, PIO_FONT, buf)) {
		strerror_r(errno, errbuf, sizeof(errbuf));
		ERR(ctx, "putfont: ioctl(PIO_FONT): %d,%dx%d: %s", count, width, height, errbuf);
		return -1;
	}

	return 0;
}
