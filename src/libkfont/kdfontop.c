// SPDX-License-Identifier: LGPL-2.0-or-later
/*
 * Copyright (C) 2007-2020 Alexey Gladkov <gladkov.alexey@gmail.com>
 *
 * Originally written by Andries Brouwer
 */
#include "config.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h> /* free() */
#include <string.h>
#include <limits.h>
#include <sys/ioctl.h>
#include <linux/kd.h>

#include "libcommon.h"
#include "kfontP.h"

#ifdef COMPAT_HEADERS
#include "compat/linux-kd.h"
#endif

int
kfont_restore_font(struct kfont_context *ctx, int fd)
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

static int
get_font_kdfontop(struct kfont_context *ctx, int consolefd,
		unsigned char *buf,
		unsigned int *count,
		unsigned int *width,
		unsigned int *height)
{
	struct console_font_op cfo;

	cfo.op = KD_FONT_OP_GET;
	cfo.flags = 0;
	cfo.width = cfo.height = 32;
	cfo.charcount = *count;
	cfo.data = buf;

	errno = 0;

	if (ioctl(consolefd, KDFONTOP, &cfo)) {
		if (errno != ENOSYS && errno != EINVAL) {
			KFONT_ERR(ctx, "ioctl(KDFONTOP): %m");
			return -1;
		}
		return 1;
	}

	*count = cfo.charcount;
	if (height)
		*height = cfo.height;
	if (width)
		*width = cfo.width;
	return 0;
}

static int
get_font_giofontx(struct kfont_context *ctx, int consolefd,
		unsigned char *buf,
		unsigned int *count,
		unsigned int *width,
		unsigned int *height)
{
	struct consolefontdesc cfd;

	if (*count > USHRT_MAX) {
		KFONT_ERR(ctx, _("GIO_FONTX: the number of characters in the font cannot be more than %d"), USHRT_MAX);
		return -1;
	}

	cfd.charcount  = (unsigned short) *count;
	cfd.charheight = 0;
	cfd.chardata   = (char *)buf;

	errno = 0;

	if (ioctl(consolefd, GIO_FONTX, &cfd)) {
		if (errno != ENOSYS && errno != EINVAL) {
			KFONT_ERR(ctx, "ioctl(GIO_FONTX): %m");
			return -1;
		}
		return 1;
	}

	*count = cfd.charcount;
	if (height)
		*height = cfd.charheight;
	if (width)
		*width = 8; /* This method do not support width != 8 */
	return 0;
}

static int
get_font_giofont(struct kfont_context *ctx, int consolefd,
		unsigned char *buf,
		unsigned int *count,
		unsigned int *width,
		unsigned int *height)
{
	if (*count != 256) {
		KFONT_ERR(ctx, _("getfont called with count<256"));
		return -1;
	}

	if (!buf) {
		KFONT_ERR(ctx, _("getfont using GIO_FONT needs buf"));
		return -1;
	}

	errno = 0;

	if (ioctl(consolefd, GIO_FONT, buf)) {
		KFONT_ERR(ctx, "ioctl(GIO_FONT): %m");
		return -1;
	}

	*count = 256;
	if (height)
		*height = 0; /* undefined, at most 32 */
	if (width)
		*width = 8; /* This method do not support width != 8 */
	return 0;
}

/*
 * May be called with buf==NULL if we only want info.
 * May be called with width==NULL and height==NULL.
 * Must not exit - we may have cleanup to do.
 */
int
kfont_get_font(struct kfont_context *ctx, int fd, unsigned char *buf,
		unsigned int *count,
		unsigned int *width,
		unsigned int *height)
{
	int ret;

	/* First attempt: KDFONTOP */
	ret = get_font_kdfontop(ctx, fd, buf, count, width, height);
	if (ret <= 0)
		return ret;

	/* Second attempt: GIO_FONTX */
	ret = get_font_giofontx(ctx, fd, buf, count, width, height);
	if (ret <= 0)
		return ret;

	/* Third attempt: GIO_FONT */
	return get_font_giofont(ctx, fd, buf, count, width, height);
}

int unsigned
kfont_get_fontsize(struct kfont_context *ctx, int fd)
{
	unsigned int count = 0;
	if (!kfont_get_font(ctx, fd, NULL, &count, NULL, NULL))
		return count;
	return 256;
}

static int
put_font_kdfontop(struct kfont_context *ctx, int consolefd, unsigned char *buf,
		unsigned int count,
		unsigned int width,
		unsigned int height)
{
	struct console_font_op cfo;

	cfo.op        = KD_FONT_OP_SET;
	cfo.flags     = 0;
	cfo.width     = width;
	cfo.height    = height;
	cfo.charcount = count;
	cfo.data      = buf;

	errno = 0;

	if (!ioctl(consolefd, KDFONTOP, &cfo))
		return 0;

	if (errno == ENOSYS)
		return 1;

	int ret = -1;

	/* In case count is not 256 or 512 round up and try again. */
	if (errno == EINVAL && width == 8 && count != 256 && count < 512) {
		unsigned int ct = ((count > 256) ? 512 : 256);
		unsigned char *mybuf = calloc(ct, 32U);

		if (!mybuf) {
			KFONT_ERR(ctx, "calloc: %m");
			return -1;
		}

		memcpy(mybuf, buf, 32U * count);

		cfo.data      = mybuf;
		cfo.charcount = ct;

		errno = 0;

		ret = ioctl(consolefd, KDFONTOP, &cfo);
		free(mybuf);
	}

	KFONT_ERR(ctx, "ioctl(KDFONTOP): %m");
	return ret;
}

static int
put_font_piofontx(struct kfont_context *ctx, int consolefd, unsigned char *buf,
		unsigned int count,
		unsigned int width,
		unsigned int height)
{
	struct consolefontdesc cfd;

	/*
	 * technically, this charcount can be up to USHRT_MAX but now there is
	 * no way to upload a font larger than 512.
	 */
	if (count > USHRT_MAX) {
		KFONT_ERR(ctx, "PIO_FONTX: the number of characters in the font cannot be more than %d", USHRT_MAX);
		return -1;
	}

	if (height > 32) {
		KFONT_ERR(ctx, "PIO_FONTX: the font height cannot be more than %d", 32);
		return -1;
	}

	cfd.charcount  = (unsigned short) count;
	cfd.charheight = (unsigned short) height;
	cfd.chardata   = (char *)buf;

	if (!ioctl(consolefd, PIO_FONTX, &cfd))
		return 0;

	if (errno != ENOSYS && errno != EINVAL) {
		KFONT_ERR(ctx, "ioctl(PIO_FONTX): %d,%dx%d: failed: %m", count, width, height);
		return -1;
	}

	return 1;
}

static int
put_font_piofont(struct kfont_context *ctx, int consolefd, unsigned char *buf,
		unsigned int count,
		unsigned int width,
		unsigned int height)
{
	if (width != 8) {
		KFONT_ERR(ctx, "PIO_FONT: unsupported font width: %d", width);
		return -1;
	}

	if (height != 32) {
		KFONT_ERR(ctx, "PIO_FONT: unsupported font height: %d", height);
		return -1;
	}

	if (count < 256) {
		KFONT_ERR(ctx, "PIO_FONT: The font is %d characters long (256 characters expected)", count);
		return -1;
	}

	if (count > 256) {
		KFONT_WARN(ctx, "PIO_FONT: The font is %d characters long but only 256 will be loaded", count);
	}

	/* This will load precisely 256 chars, independent of count */
	if (!ioctl(consolefd, PIO_FONT, buf))
		return 0;

	KFONT_ERR(ctx, "ioctl(PIO_FONT): %m");
	return -1;
}


int
kfont_put_font(struct kfont_context *ctx, int fd, unsigned char *buf, unsigned int count,
        unsigned int width, unsigned int height)
{
	int ret;

	if (!width)
		width = 8;

	if (!height)
		height = font_charheight(buf, count, width);

	/* First attempt: KDFONTOP */
	ret = put_font_kdfontop(ctx, fd, buf, count, width, height);
	if (ret <= 0)
		return ret;

	/* Second attempt: PIO_FONTX */
	ret = put_font_piofontx(ctx, fd, buf, count, width, height);
	if (ret <= 0)
		return ret;

	/* Third attempt: PIO_FONT */
	return put_font_piofont(ctx, fd, buf, count, width, height);
}
