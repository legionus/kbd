/*
 * Read kernel font into BUF with room for COUNT 32x32 glyphs.
 * Return 0 on success -1 on failure.
 * Sets number of glyphs in COUNT, glyph height in HEIGHT.
 */
extern int getfont(int fd, char *buf, int *count, int *height);

/*
 * Load kernel font of pointsize HEIGHT from BUF with length COUNT.
 * Return 0 on success, -1 on failure.
 */
extern int putfont(int fd, char *buf, int count, int height);

/*
 * Find the maximum height of nonblank pixels (in the 32*COUNT bytes of BUF).
 */
extern int font_charheight(char *buf, int count);
