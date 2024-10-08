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
#define GIO_FONT 0x4B60
#define PIO_FONT 0x4B61
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
#define GIO_FONTX 0x4B6B
#define PIO_FONTX 0x4B6C
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
#define PIO_FONTRESET 0x4B6D /* reset to default font */
#endif

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
	unsigned int op;    /* KD_FONT_OP_* */
	unsigned int flags; /* KD_FONT_FLAG_* */
	unsigned int width, height;
	unsigned int charcount;
	unsigned char *data; /* font data with vpitch fixed to 32 for
                              * KD_FONT_OP_SET/GET */
};

#define KD_FONT_OP_SET 0         /* Set font */
#define KD_FONT_OP_GET 1         /* Get font */
#define KD_FONT_OP_SET_DEFAULT 2 /* Set font to default, \
                                    data points to name / NULL */
#define KD_FONT_OP_COPY 3        /* Copy from another console */
#define KD_FONT_OP_SET_TALL 4    /* Set font with arbitrary vpitch */
#define KD_FONT_OP_GET_TALL 5    /* Get font with arbitrary vpitch */

#define KD_FONT_FLAG_OLD 0x80000000 /* Invoked via old interface */
#define KD_FONT_FLAG_DONT_RECALC 1  /* Don't call adjust_height() */
                                    /* (Used internally for PIO_FONT support) */
#endif                              /* KDFONTOP */

#ifndef KDKBDREP
/* usually defined in <linux/kd.h> */
/* set keyboard delay/repeat rate;
 * actually used values are returned */
#define KDKBDREP 0x4B52
#endif
