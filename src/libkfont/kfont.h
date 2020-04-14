#ifndef _FONT_H_
#define _FONT_H_

#include "context.h"

/* mapscrn.c */

int loadnewmap(struct kfont_context *ctx, int fd, const char *mfil)
	__attribute__((nonnull(1)));

int saveoldmap(struct kfont_context *ctx, int fd, const char *omfil)
	__attribute__((nonnull(1)));

/* loadunimap.c */

/* save humanly readable */
int saveunicodemap(struct kfont_context *ctx, int fd, char *oufil)
	__attribute__((nonnull(1)));

int loadunicodemap(struct kfont_context *ctx, int fd, const char *ufil)
	__attribute__((nonnull(1)));

int appendunicodemap(struct kfont_context *ctx, int fd, FILE *fp,
		unsigned int ct, int utf8)
	__attribute__((nonnull(1)));

/* kdfontop.c */

/*
 * Read kernel font into BUF with room for COUNT 32x32 glyphs.
 * Return 0 on success -1 on failure.
 * Sets number of glyphs in COUNT, glyph size in WIDTH and HEIGHT.
 */
int getfont(struct kfont_context *ctx, int fd, unsigned char *buf,
		unsigned int *count, unsigned int *width, unsigned int *height)
	__attribute__((nonnull(1)));

/*
 * Load kernel font of width WIDTH and pointsize HEIGHT from BUF
 * with length COUNT.
 * Return 0 on success, -1 on failure.
 */
int putfont(struct kfont_context *ctx, int fd, unsigned char *buf,
		unsigned int count, unsigned int width, unsigned int height)
	__attribute__((nonnull(1)));

/*
 * Find the maximum height of nonblank pixels
 * (in the ((WIDTH+7)/8)*32*COUNT bytes of BUF).
 */
unsigned int font_charheight(unsigned char *buf, unsigned int count,
		unsigned int width)
	__attribute__((nonnull(1)));

/*
 * Find the size of the kernel font.
 */
unsigned int getfontsize(struct kfont_context *ctx, int fd)
	__attribute__((nonnull(1)));

/*
 * Restore font (doesn't work).
 */
int restorefont(struct kfont_context *ctx, int fd)
	__attribute__((nonnull(1)));

/* kdmapop.c */

int getscrnmap(struct kfont_context *ctx, int fd, unsigned char *map)
	__attribute__((nonnull(1)));

int loadscrnmap(struct kfont_context *ctx, int fd, unsigned char *map)
	__attribute__((nonnull(1)));

int getuniscrnmap(struct kfont_context *ctx, int fd, unsigned short *map)
	__attribute__((nonnull(1)));

int loaduniscrnmap(struct kfont_context *ctx, int fd, unsigned short *map)
	__attribute__((nonnull(1)));

#include <linux/kd.h>

int getunimap(struct kfont_context *ctx, int fd, struct unimapdesc *ud)
	__attribute__((nonnull(1)));

int loadunimap(struct kfont_context *ctx, int fd, struct unimapinit *ui,
		struct unimapdesc *ud)
	__attribute__((nonnull(1)));

/* setfont.c */
int saveoldfontplusunicodemap(struct kfont_context *ctx, int fd, const char *Ofil)
	__attribute__((nonnull(1)));

int saveoldfont(struct kfont_context *ctx, int fd, const char *ofil)
	__attribute__((nonnull(1)));

int loadnewfont(struct kfont_context *ctx,
		int fd, const char *ifil,
		unsigned int iunit, unsigned int hwunit, int no_m, int no_u)
	__attribute__((nonnull(1)));

int loadnewfonts(struct kfont_context *ctx,
		int fd, const char *const *ifiles, int ifilct,
		unsigned int iunit, unsigned int hwunit, int no_m, int no_u)
	__attribute__((nonnull(1)));

void activatemap(int fd);
void disactivatemap(int fd);

#endif /* _FONT_H_ */
