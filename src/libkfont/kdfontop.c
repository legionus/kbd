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
#include "kfontP.h"

#ifdef COMPAT_HEADERS
#include "compat/linux-kd.h"
#endif

int
kfont_restorefont(struct kfont_context *ctx, int fd)
{
	if (ioctl(fd, PIO_FONTRESET, 0)) {
		KFONT_ERR(ctx, "ioctl(PIO_FONTRESET): %m");
		return -1;
	}
	return 0;
}

unsigned int
font_charheight(unsigned char *buf, unsigned int count, unsigned int width)
{
	unsigned int h, i, x;
	unsigned int bytewidth = (width + 7) / 8;

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
kfont_getfont(struct kfont_context *ctx, int fd, unsigned char *buf, unsigned int *count,
        unsigned int *width, unsigned int *height)
{
	struct consolefontdesc cfd;
	struct console_font_op cfo;
	int i;

	/* First attempt: KDFONTOP */
	cfo.op = KD_FONT_OP_GET;
	cfo.flags = 0;
	cfo.width = cfo.height = 32;
	cfo.charcount = *count;
	cfo.data = buf;

	i = ioctl(fd, KDFONTOP, &cfo);
	if (!i) {
		*count = cfo.charcount;
		if (height)
			*height = cfo.height;
		if (width)
			*width = cfo.width;
		return 0;
	}

	if (errno != ENOSYS && errno != EINVAL) {
		KFONT_ERR(ctx, "ioctl(KDFONTOP): %m");
		return -1;
	}

	/* The other methods do not support width != 8 */
	if (width)
		*width = 8;
	/* Second attempt: GIO_FONTX */
	cfd.charcount  = *count;
	cfd.charheight = 0;
	cfd.chardata   = (char *)buf;

	i = ioctl(fd, GIO_FONTX, &cfd);
	if (!i) {
		*count = cfd.charcount;
		if (height)
			*height = cfd.charheight;
		return 0;
	}

	if (errno != ENOSYS && errno != EINVAL) {
		KFONT_ERR(ctx, "ioctl(GIO_FONTX): %m");
		return -1;
	}

	/* Third attempt: GIO_FONT */
	if (*count < 256) {
		KFONT_ERR(ctx, _("bug: getfont called with count<256"));
		return -1;
	}

	if (!buf) {
		KFONT_ERR(ctx, _("bug: getfont using GIO_FONT needs buf."));
		return -1;
	}

	i = ioctl(fd, GIO_FONT, buf);
	if (i) {
		KFONT_ERR(ctx, "ioctl(GIO_FONT): %m");
		return -1;
	}

	*count = 256;
	if (height)
		*height = 0; /* undefined, at most 32 */

	return 0;
}

int unsigned
kfont_getfontsize(struct kfont_context *ctx, int fd)
{
	unsigned int count = 0;
	if (!kfont_getfont(ctx, fd, NULL, &count, NULL, NULL))
		return count;
	return 256;
}

int
kfont_putfont(struct kfont_context *ctx, int fd, unsigned char *buf, unsigned int count,
        unsigned int width, unsigned int height)
{
	struct consolefontdesc cfd;
	struct console_font_op cfo;
	int i;

	if (!width)
		width = 8;

	if (!height)
		height = font_charheight(buf, count, width);

	/* First attempt: KDFONTOP */
	cfo.op        = KD_FONT_OP_SET;
	cfo.flags     = 0;
	cfo.width     = width;
	cfo.height    = height;
	cfo.charcount = count;
	cfo.data      = buf;

	i = ioctl(fd, KDFONTOP, &cfo);
	if (!i)
		return 0;

	if (width != 8 || (errno != ENOSYS && errno != EINVAL)) {
		KFONT_ERR(ctx, "ioctl(KDFONTOP): %m");
		return -1;
	}

	/* Variation on first attempt: in case count is not 256 or 512
	   round up and try again. */
	if (errno == EINVAL && width == 8 && count != 256 && count < 512) {
		unsigned int ct = ((count > 256) ? 512 : 256);
		unsigned char *mybuf = malloc(32U * ct);

		if (!mybuf) {
			KFONT_ERR(ctx, "malloc: %m");
			return -1;
		}

		memset(mybuf, 0, 32U * ct);
		memcpy(mybuf, buf, 32U * count);

		cfo.data      = mybuf;
		cfo.charcount = ct;

		i = ioctl(fd, KDFONTOP, &cfo);
		free(mybuf);

		if (i == 0)
			return 0;
	}

	/* Second attempt: PIO_FONTX */
	cfd.charcount  = count;
	cfd.charheight = height;
	cfd.chardata   = (char *)buf;

	i = ioctl(fd, PIO_FONTX, &cfd);
	if (!i)
		return 0;

	if (errno != ENOSYS && errno != EINVAL) {
		KFONT_ERR(ctx, "ioctl(PIO_FONTX): %d,%dx%d: failed: %m", count, width, height);
		return -1;
	}

	/* Third attempt: PIO_FONT */
	/* This will load precisely 256 chars, independent of count */
	i = ioctl(fd, PIO_FONT, buf);
	if (i) {
		KFONT_ERR(ctx, "ioctl(PIO_FONT): %d,%dx%d: failed: %m", count, width, height);
		return -1;
	}

	return 0;
}
