/*
 * kdfontop.c - export getfont(), getfontsize() and putfont()
 *
 * Font handling differs between various kernel versions.
 * Hide the differences in this file.
 */
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>		/* free() */
#include <unistd.h>		/* usleep() */
#include <sys/ioctl.h>
#include <linux/kd.h>
#include "kdfontop.h"
#include "nls.h"
#include "version.h"

#ifdef COMPAT_HEADERS
#include "compat/linux-kd.h"
#endif

int
restorefont(int fd) {
	if (ioctl(fd, PIO_FONTRESET, 0)) {
		perror("PIO_FONTRESET");
		return -1;
	}
	return 0;
}

int
font_charheight(unsigned char *buf, int count, int width) {
	int h, i, x;
	int bytewidth = (width+7)/8;

	for (h = 32; h > 0; h--)
		for (i = 0; i < count; i++)
			for (x = 0; x < bytewidth; x++)
				if (buf[(32*i+h-1)*bytewidth+x])
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
getfont(int fd, unsigned char *buf, int *count, int *width, int *height) {
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
	if (i == 0) {
		*count = cfo.charcount;
		if (height)
			*height = cfo.height;
		if (width)
			*width = cfo.width;
		return 0;
	}
	if (errno != ENOSYS && errno != EINVAL) {
		perror("getfont: KDFONTOP");
		return -1;
	}

	/* The other methods do not support width != 8 */
	if (width) *width = 8;
	/* Second attempt: GIO_FONTX */
	cfd.charcount = *count;
	cfd.charheight = 0;
	cfd.chardata = (char *)buf;
	i = ioctl(fd, GIO_FONTX, &cfd);
	if (i == 0) {
		*count = cfd.charcount;
		if (height)
			*height = cfd.charheight;
		return 0;
	}
	if (errno != ENOSYS && errno != EINVAL) {
		perror("getfont: GIO_FONTX");
		return -1;
	}

	/* Third attempt: GIO_FONT */
	if (*count < 256) {
		fprintf(stderr, _("bug: getfont called with count<256\n"));
		return -1;
	}
	if (!buf) {
	    fprintf(stderr, _("bug: getfont using GIO_FONT needs buf.\n"));
	    return -1;
	}
	i = ioctl(fd, GIO_FONT, buf);
	if (i) {
		perror("getfont: GIO_FONT");
		return -1;
	}
	*count = 256;
	if (height)
		*height = 0;	/* undefined, at most 32 */
	return 0;
}

int
getfontsize(int fd) {
	int count;
	int i;

	count = 0;
	i = getfont(fd, NULL, &count, NULL, NULL);
	return (i == 0) ? count : 256;
}

int
putfont(int fd, unsigned char *buf, int count, int width, int height) {
	struct consolefontdesc cfd;
	struct console_font_op cfo;
	int i;

	if (!width)
		width = 8;
	if (!height)
		height = font_charheight(buf, count, width);

	/* First attempt: KDFONTOP */
	cfo.op = KD_FONT_OP_SET;
	cfo.flags = 0;
	cfo.width = width;
	cfo.height = height;
	cfo.charcount = count;
	cfo.data = buf;
	i = ioctl(fd, KDFONTOP, &cfo);
	if (i == 0)
		return 0;
	if (width != 8 || (errno != ENOSYS && errno != EINVAL)) {
		perror("putfont: KDFONTOP");
		return -1;
	}

	/* Variation on first attempt: in case count is not 256 or 512
	   round up and try again. */
	if (errno == EINVAL && width == 8 && count != 256 && count < 512) {
		int ct = ((count > 256) ? 512 : 256);
		unsigned char *mybuf = malloc(32 * ct);

		if (!mybuf) {
			fprintf(stderr, _("%s: out of memory\n"), progname);
			return -1;
		}
		memset(mybuf, 0, 32 * ct);
		memcpy(mybuf, buf, 32 * count);
		cfo.data = mybuf;
		cfo.charcount = ct;
		i = ioctl(fd, KDFONTOP, &cfo);
		free(mybuf);
		if (i == 0)
			return 0;
	}

	/* Second attempt: PIO_FONTX */
	cfd.charcount = count;
	cfd.charheight = height;
	cfd.chardata = (char *)buf;
	i = ioctl(fd, PIO_FONTX, &cfd);
	if (i == 0)
		return 0;
	if (errno != ENOSYS && errno != EINVAL) {
		fprintf(stderr, "%s: putfont: %d,%dx%d:failed: %d\n", progname, count, width, height, i);
		perror("putfont: PIO_FONTX");
		return -1;
	}

	/* Third attempt: PIO_FONT */
	/* This will load precisely 256 chars, independent of count */
	int loop = 0;

	/* we allow ourselves to hang here for ca 5 seconds, xdm may be playing tricks on us. */
	while ((loop++ < 20) && (i = ioctl(fd, PIO_FONT, buf)))
          {
	    if (loop <= 1)
	      fprintf(stderr, "putfont: PIO_FONT trying ...\n");
	    else
	      fprintf(stderr, ".");
	    usleep(250000);
	  }
	fprintf(stderr, "\n");

	if (i) {
		fprintf(stderr, "%s: putfont: %d,%dx%d:  failed: %d\n", progname, count, width, height, i);
		perror("putfont: PIO_FONT");
		return -1;
	}
	return 0;
}
