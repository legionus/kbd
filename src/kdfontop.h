#ifndef _KDFONTOP_H
#define _KDFONTOP_H
/*
 * Read kernel font into BUF with room for COUNT 32x32 glyphs.
 * Return 0 on success -1 on failure.
 * Sets number of glyphs in COUNT, glyph size in WIDTH and HEIGHT.
 */
extern int getfont(int fd, unsigned char *buf, int *count, int *width, int *height);

/*
 * Load kernel font of width WIDTH and pointsize HEIGHT from BUF
 * with length COUNT.
 * Return 0 on success, -1 on failure.
 */
extern int putfont(int fd, unsigned char *buf, int count, int width, int height);

/*
 * Find the maximum height of nonblank pixels
 * (in the ((WIDTH+7)/8)*32*COUNT bytes of BUF).
 */
extern int font_charheight(unsigned char *buf, int count, int width);

/*
 * Find the size of the kernel font.
 */
extern int getfontsize(int fd);

/*
 * Restore font (doesn't work).
 */
extern int restorefont(int fd);

#endif /* _KDFONTOP_H */
