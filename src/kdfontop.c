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

/*
 * Linux pre-0.96 introduced, and 1.1.63 removed the defines
 * #define GIO_FONT8x8     0x4B28
 * #define PIO_FONT8x8     0x4B29
 * #define GIO_FONT8x14    0x4B2A
 * #define PIO_FONT8x14    0x4B2B
 * #define GIO_FONT8x16    0x4B2C
 * #define PIO_FONT8x16    0x4B2D
 * but these ioctls have never been implemented.
 */

/*
 * Linux 0.99.14y introduces the GIO_FONT and PIO_FONT ioctls.
 * Usage:
	char buf[8192];
	ioctl(fd, GIO_FONT, buf);
 * to get 256*32=8192 bytes of data for 256 characters,
 * 32 for each symbol, of which only the first H are used
 * for an 8xH font.
 * Changes in use: 1.1.74: you have to be root for PIO_FONT.
 */
#ifndef GIO_FONT
#define GIO_FONT        0x4B60
#define PIO_FONT        0x4B61
#endif

/*
 * Linux 1.3.1 introduces 512-character fonts and the
 * GIO_FONTX and PIO_FONTX ioctls to read and load them.
 * The PIO_FONTX ioctl also adjusts screen character height.
 * Usage:
	char buf[16384];
	struct consolefontdesc cfd;
	cfd.charcount = fontsize;
	cfd.charheight = height;
	cfd.chardata = buf;
	ioctl(fd, PIO_FONTX, &cfd);
 * and
	char buf[32*N];
	cfd.charcount = N;
	cfd.chardata = buf;
	ioctl(fd, GIO_FONTX, &cfd);
 * (where the ioctl will fail if N was too small);
 * the ioctl fills cfd.charcount and cfd.charheight.
 * With GIO_FONTX, the chardata pointer may be NULL.
 * The old GIO_FONT will fail if the fontsize is 512.
 */
#ifndef GIO_FONTX
#define GIO_FONTX  0x4B6B
#define PIO_FONTX  0x4B6C
struct consolefontdesc {
	unsigned short charcount;
	unsigned short charheight;
	char *chardata;
};
#endif

/*
 * Linux 1.3.28 introduces the PIO_FONTRESET ioctl.
 * Usage:
	ioctl(fd, PIO_FONTRESET, 0);
 * The default font is kept in slot 0 of the video card character ROM,
 * and is never touched.
 * A custom font is loaded in slot 2 (256 char) or 2:3 (512 char).
 *
 * However, 1.3.30 takes this away again by hiding it behind
 * #ifndef BROKEN_GRAPHICS_PROGRAMS, while in fact this variable
 * is defined (in vt_kern.h).  Now by default every font lives in
 * slot 0 (256 char) or 0:1 (512 char).
 * And these days (2.2pre), even if BROKEN_GRAPHICS_PROGRAMS is undefined,
 * the PIO_FONTRESET does not work since it is not implemented for vgacon.
 *
 * In other words, this ioctl is totally useless today.
 */
#ifndef PIO_FONTRESET
#define PIO_FONTRESET   0x4B6D  /* reset to default font */
#endif

int
restorefont(int fd) {
	if (ioctl(fd, PIO_FONTRESET, 0)) {
		perror("PIO_FONTRESET");
		return -1;
	}
	return 0;
}

/*
 * Linux 2.1.111 introduces the KDFONTOP ioctl.
 * Details of use have changed a bit in 2.1.111-115,124.
 * Usage:
	struct console_font_op cfo;
	ioctl(fd, KDFONTOP, &cfo);
 */
#ifndef KDFONTOP
#define KDFONTOP 0x4B72
struct console_font_op {
        unsigned int op;	/* KD_FONT_OP_* */
        unsigned int flags;	/* KD_FONT_FLAG_* */
        unsigned int width, height;
        unsigned int charcount;
        unsigned char *data;	/* font data with height fixed to 32 */
};

#define KD_FONT_OP_SET          0     /* Set font */
#define KD_FONT_OP_GET          1     /* Get font */
#define KD_FONT_OP_SET_DEFAULT  2     /* Set font to default,
					 data points to name / NULL */
#define KD_FONT_OP_COPY         3     /* Copy from another console */

#define KD_FONT_FLAG_OLD	0x80000000 /* Invoked via old interface */
#define KD_FONT_FLAG_DONT_RECALC 1    /* Don't call adjust_height() */
			  /* (Used internally for PIO_FONT support) */
#endif /* KDFONTOP */

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
