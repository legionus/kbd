#ifndef _FONT_H_
#define _FONT_H_

#include "context.h"

/* mapscrn.c */

int loadnewmap(struct kfont_context *ctx, int fd, const char *mfil);
void saveoldmap(struct kfont_context *ctx, int fd, const char *omfil);

/* loadunimap.c */

void saveunicodemap(struct kfont_context *ctx, int fd, char *oufil); /* save humanly readable */
void loadunicodemap(struct kfont_context *ctx, int fd, const char *ufil);
void appendunicodemap(struct kfont_context *ctx, int fd, FILE *fp,
		unsigned int ct, int utf8);

/* kdfontop.c */

/*
 * Read kernel font into BUF with room for COUNT 32x32 glyphs.
 * Return 0 on success -1 on failure.
 * Sets number of glyphs in COUNT, glyph size in WIDTH and HEIGHT.
 */
int getfont(struct kfont_context *ctx, int fd, unsigned char *buf,
		unsigned int *count, unsigned int *width, unsigned int *height);

/*
 * Load kernel font of width WIDTH and pointsize HEIGHT from BUF
 * with length COUNT.
 * Return 0 on success, -1 on failure.
 */
int putfont(struct kfont_context *ctx, int fd, unsigned char *buf,
		unsigned int count, unsigned int width, unsigned int height);

/*
 * Find the maximum height of nonblank pixels
 * (in the ((WIDTH+7)/8)*32*COUNT bytes of BUF).
 */
unsigned int font_charheight(unsigned char *buf, unsigned int count,
		unsigned int width);

/*
 * Find the size of the kernel font.
 */
unsigned int getfontsize(struct kfont_context *ctx, int fd);

/*
 * Restore font (doesn't work).
 */
int restorefont(struct kfont_context *ctx, int fd);

/* kdmapop.c */

int getscrnmap(struct kfont_context *ctx, int fd, unsigned char *map);
int loadscrnmap(struct kfont_context *ctx, int fd, unsigned char *map);
int getuniscrnmap(struct kfont_context *ctx, int fd, unsigned short *map);
int loaduniscrnmap(struct kfont_context *ctx, int fd, unsigned short *map);
int getunimap(struct kfont_context *ctx, int fd, struct unimapdesc *ud);
int loadunimap(struct kfont_context *ctx, int fd, struct unimapinit *ui,
		struct unimapdesc *ud);

#endif /* _FONT_H_ */
