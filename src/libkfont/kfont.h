/**
 * @file kfont.h
 * @brief Functions for search, open and close a file objects.
 */
#ifndef _KFONT_H_
#define _KFONT_H_

#include <stdio.h>
#include <stdarg.h>
#include <sys/param.h>

#ifndef __GNUC__
#define __attribute__(x) /*NOTHING*/
#endif

typedef void (*kfont_logger_t)(void *, int, const char *, int, const char *, const char *, va_list) __attribute__((format(printf, 6, 0)));

struct kfont_ctx;

struct kfont_ctx *kfont_context_new(void);
void *kfont_context_free(struct kfont_ctx *ctx);

kfont_logger_t kfont_get_log_fn(struct kfont_ctx *ctx);
int kfont_set_log_fn(struct kfont_ctx *ctx, kfont_logger_t log_fn, const void *data);

void *kfont_get_log_data(struct kfont_ctx *ctx);
int kfont_set_log_data(struct kfont_ctx *ctx, const void *data);

int kfont_get_log_priority(struct kfont_ctx *ctx);
int kfont_set_log_priority(struct kfont_ctx *ctx, int priority);

#include <syslog.h>

void kfont_log(struct kfont_ctx *ctx, int priority,
            const char *file, int line, const char *fn,
            const char *fmt, ...);

/* kdfontop.c */
/*
 * Read kernel font into BUF with room for COUNT 32x32 glyphs.
 * Return 0 on success -1 on failure.
 * Sets number of glyphs in COUNT, glyph size in WIDTH and HEIGHT.
 */
int kfont_getfont(struct kfont_ctx *ctx, int fd, unsigned char *buf, size_t *count, size_t *width, size_t *height);

/*
 * Load kernel font of width WIDTH and pointsize HEIGHT from BUF
 * with length COUNT.
 * Return 0 on success, -1 on failure.
 */
int kfont_putfont(struct kfont_ctx *ctx, int fd, unsigned char *buf, size_t count, size_t width, size_t height);

/*
 * Find the maximum height of nonblank pixels
 * (in the ((WIDTH+7)/8)*32*COUNT bytes of BUF).
 */
size_t kfont_font_charheight(unsigned char *buf, size_t count, size_t width);

/*
 * Find the size of the kernel font.
 */
size_t kfont_getfontsize(struct kfont_ctx *ctx, int fd);

/*
 * Restore font (doesn't work).
 */
int kfont_restorefont(struct kfont_ctx *ctx, int fd);

/* kdmapop.c */
#include <linux/kd.h>

int kfont_getscrnmap(struct kfont_ctx *ctx, int fd, char *map);
int kfont_loadscrnmap(struct kfont_ctx *ctx, int fd, char *map);
int kfont_getuniscrnmap(struct kfont_ctx *ctx, int fd, unsigned short *map);
int kfont_loaduniscrnmap(struct kfont_ctx *ctx, int fd, unsigned short *map);
int kfont_getunimap(struct kfont_ctx *ctx, int fd, struct unimapdesc *ud);
int kfont_loadunimap(struct kfont_ctx *ctx, int fd, struct unimapinit *ui, struct unimapdesc *ud);

/* loadunimap.c */
int kfont_saveunicodemap(struct kfont_ctx *ctx, int fd, char *oufil);
int kfont_loadunicodemap(struct kfont_ctx *ctx, int fd, const char *ufil, const char *const *unidirpath, const char *const *unisuffixes);
int kfont_appendunicodemap(struct kfont_ctx *ctx, int fd, FILE *fp, size_t fontsize, int utf8);

/* mapscrn.c */
int kfont_saveoldmap(struct kfont_ctx *ctx, int fd, char *omfil);
int kfont_loadnewmap(struct kfont_ctx *ctx, int fd, char *mfil, const char *const *mapdirpath, const char *const *mapsuffixes);

#endif /* _KFONT_H_ */
