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

/**
 * @brief Parser flags that are set outside the library.
 */
typedef enum {
	KFONT_FLAG_SKIP_LOAD_UNICODE_MAP = (1 << 1),
	KFONT_FLAG_SKIP_LOAD_SCREEN_MAP  = (1 << 2),
} kfont_flags;

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

kfont_flags kfont_get_flags(struct kfont_ctx *ctx);
int kfont_set_flags(struct kfont_ctx *ctx, kfont_flags flags);
int kfont_set_console(struct kfont_ctx *ctx, int fd);

int kfont_set_fontdirs(struct kfont_ctx *ctx, char **dirs, char **suffixes);
int kfont_set_partfontdirs(struct kfont_ctx *ctx, char **dirs, char **suffixes);
int kfont_set_mapdirs(struct kfont_ctx *ctx, char **dirs, char **suffixes);
int kfont_set_unidirs(struct kfont_ctx *ctx, char **dirs, char **suffixes);

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
int kfont_get_font(struct kfont_ctx *ctx, unsigned char *buf, size_t *count, size_t *width, size_t *height);

/*
 * Load kernel font of width WIDTH and pointsize HEIGHT from BUF
 * with length COUNT.
 * Return 0 on success, -1 on failure.
 */
int kfont_load_font(struct kfont_ctx *ctx, unsigned char *buf, size_t count, size_t width, size_t height);

/*
 * Find the maximum height of nonblank pixels
 * (in the ((WIDTH+7)/8)*32*COUNT bytes of BUF).
 */
size_t kfont_get_charheight(unsigned char *buf, size_t count, size_t width);

/*
 * Find the size of the kernel font.
 */
size_t kfont_get_fontsize(struct kfont_ctx *ctx);

/*
 * Restore font (doesn't work).
 */
int kfont_restore_font(struct kfont_ctx *ctx);

/* kdmapop.c */

#include <linux/kd.h>

int kfont_get_scrnmap(struct kfont_ctx *ctx, char *map);
int kfont_load_scrnmap(struct kfont_ctx *ctx, char *map);

int kfont_get_uniscrnmap(struct kfont_ctx *ctx, unsigned short *map);
int kfont_load_uniscrnmap(struct kfont_ctx *ctx, unsigned short *map);

int kfont_get_unimap(struct kfont_ctx *ctx, struct unimapdesc *ud);
int kfont_load_unimap(struct kfont_ctx *ctx, struct unimapinit *ui, struct unimapdesc *ud);

/* loadunimap.c */

int kfont_dump_unicodemap(struct kfont_ctx *ctx, char *oufil);
int kfont_load_unicodemap(struct kfont_ctx *ctx, const char *ufil);

/* mapscrn.c */

int kfont_dump_map(struct kfont_ctx *ctx, char *omfil);
int kfont_load_map(struct kfont_ctx *ctx, char *mfil);

/* psffontop.c */

/* Maximum font size that we try to handle */
#define MAXFONTSIZE 65536

typedef unsigned int unicode;

struct unicode_seq {
	struct unicode_seq *next;
	struct unicode_seq *prev;
	unicode uc;
};

struct unicode_list {
	struct unicode_list *next;
	struct unicode_list *prev;
	struct unicode_seq *seq;
};

int kfont_read_psffont(struct kfont_ctx *ctx,
                      FILE *filename, char **allbufp, size_t *allszp,
                      char **fontbufp, size_t *fontszp,
                      size_t *fontwidthp, size_t *fontlenp, size_t fontpos0,
                      struct unicode_list **uclistheadsp);

int kfont_write_psffont(struct kfont_ctx *ctx,
                       FILE *filename, char *fontbuf,
                       size_t width, size_t height, size_t fontlen, int psftype,
                       struct unicode_list *uclistheads);

#define WPSFH_HASTAB 1
#define WPSFH_HASSEQ 2
int kfont_write_psffontheader(struct kfont_ctx *ctx, FILE *ofil,
                             size_t width, size_t height, size_t fontlen,
                             int *psftype, int flags);

#include <psf.h>

/* setfont.c */
int kfont_load_fonts(struct kfont_ctx *ctx, char **ifiles, int ifilct, size_t iunit, size_t hwunit);

/* dumpfont.c */
int kfont_dump_font(struct kfont_ctx *ctx, char *filename);
int kfont_dump_fullfont(struct kfont_ctx *ctx, char *filename);

#endif /* _KFONT_H_ */
