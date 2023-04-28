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
		unsigned int *height,
		unsigned int *vpitch)
{
	struct console_font_op cfo;

#ifdef KD_FONT_OP_GET_TALL
	cfo.op = KD_FONT_OP_GET_TALL;
#else
	cfo.op = KD_FONT_OP_GET;
#endif
	cfo.flags = 0;
	cfo.width = 64;
	cfo.height = 128;
	cfo.charcount = *count;
	cfo.data = buf;

	while (1) {
		errno = 0;

		if (ioctl(consolefd, KDFONTOP, &cfo)) {
#ifdef KD_FONT_OP_GET_TALL
			if (errno == ENOSYS && cfo.op == KD_FONT_OP_GET_TALL) {
				/* Kernel before 6.2.  */
				cfo.op = KD_FONT_OP_GET;
				continue;
			}
#endif
			if (errno != ENOSYS && errno != EINVAL) {
				KFONT_ERR(ctx, "ioctl(KDFONTOP): %m");
				return -1;
			}
			return 1;
		}
		break;
	}


	*count = cfo.charcount;
	if (height)
		*height = cfo.height;
	if (width)
		*width = cfo.width;
	if (vpitch) {
#ifdef KD_FONT_OP_GET_TALL
		if (cfo.op == KD_FONT_OP_GET_TALL)
			*vpitch = cfo.height;
		else
#endif
			*vpitch = 32;
	}
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
		unsigned int *height,
		unsigned int *vpitch)
{
	return get_font_kdfontop(ctx, fd, buf, count, width, height, vpitch);
}

int unsigned
kfont_get_fontsize(struct kfont_context *ctx, int fd)
{
	unsigned int count = 0;
	if (!kfont_get_font(ctx, fd, NULL, &count, NULL, NULL, NULL))
		return count;
	return 256;
}

static int
put_font_kdfontop(struct kfont_context *ctx, int consolefd, unsigned char *buf,
		unsigned int count,
		unsigned int width,
		unsigned int height,
		unsigned int vpitch)
{
	struct console_font_op cfo;

	if (vpitch == 32 && width <= 32)
		cfo.op        = KD_FONT_OP_SET;
	else {
#ifdef KD_FONT_OP_SET_TALL
		cfo.op        = KD_FONT_OP_SET_TALL;
#else
		return 0;
#endif
	}
	cfo.flags     = 0;
	cfo.width     = width;
	cfo.height    = height;
	cfo.charcount = count;
	cfo.data      = buf;

	errno = 0;

	if (!ioctl(consolefd, KDFONTOP, &cfo))
		return 0;

	if (errno == ENOSYS) {
#ifdef KD_FONT_OP_SET_TALL
		if (cfo.op == KD_FONT_OP_SET_TALL)
			/* Let user know that we can't load such font with such kernel version */
			return 0;
#endif
		return 1;
	}

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

int
kfont_put_font(struct kfont_context *ctx, int fd, unsigned char *buf, unsigned int count,
        unsigned int width, unsigned int height, unsigned int vpitch)
{
	if (!width)
		width = 8;

	if (!height)
		height = font_charheight(buf, count, width);

	return put_font_kdfontop(ctx, fd, buf, count, width, height, vpitch);
}
