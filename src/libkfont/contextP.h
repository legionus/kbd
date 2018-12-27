#ifndef KFONT_CONTEXTP_H
#define KFONT_CONTEXTP_H

#include <stdarg.h>
#include <syslog.h>

#include <kbdfile.h>
#include <kfont.h>

#include "libcommon.h"

/**
 * @brief Opaque object representing the library context.
 */
struct kfont_ctx {
	int verbose;

	struct kbdfile_ctx *kbdfile_ctx;

	/**
	 * User defined logging function.
	 */
	kfont_logger_t log_fn;
	/**
	 * The data passed to the @ref log_fn logging function as the first argument.
	 */
	void *log_data;

	/**
	 * Logging priority used by @ref log_fn logging function.
	 */
	int log_priority;

	int flags;

	int consolefd;

	/**
	 * Search for the font in these directories (with trailing /)
	 */
	char **fontdirs;
	char **fontsuffixes;

	/**
	 * Hide partial fonts a bit - loading a single one is a bad idea
	 */
	char **partfontdirs;
	char **partfontsuffixes;

	/**
	 * Search for the map file in these directories (with trailing /)
	 */
	char **mapdirs;
	char **mapsuffixes;

	char **unidirs;
	char **unisuffixes;
};

#define STACKBUF_LEN 256

#define kfont_log_cond(ctx, level, arg...)                                          \
	do {                                                                        \
		if (ctx->log_priority >= level)                                     \
			kfont_log(ctx, level, __FILE__, __LINE__, __func__, ##arg); \
	} while (0)

/**
 * Wrapper to output debug-level messages
 * @param ctx is a kfont library context.
 * @param arg is output message.
 */
#define DBG(ctx, arg...) kfont_log_cond(ctx, LOG_DEBUG, ##arg)

/**
 * Wrapper to output informational messages
 * @param ctx is a kfont library context.
 * @param arg is output message.
 */
#define INFO(ctx, arg...) kfont_log_cond(ctx, LOG_INFO, ##arg)

/**
 * Wrapper to output warning conditions
 * @param ctx is a kfont library context.
 * @param arg is output message.
 */
#define WARN(ctx, arg...) kfont_log_cond(ctx, LOG_WARNING, ##arg)

/**
 * Wrapper to output error conditions
 * @param ctx is a kfont library context.
 * @param arg is output message.
 */
#define ERR(ctx, arg...) kfont_log_cond(ctx, LOG_ERR, ##arg)

/* loadunimap.c */
int append_unicodemap(struct kfont_ctx *ctx, FILE *fp, size_t fontsize, int utf8);

/* utf8.c */
#define UTF8_BAD   (-1)
#define UTF8_SHORT (-2)

unsigned long from_utf8(char **inptr, int cnt, int *err);

/* psffontop.c */
int appendunicode(struct kfont_ctx *ctx, FILE *fp, unsigned int uc, int utf8);
int appendseparator(struct kfont_ctx *ctx, FILE *fp, int seq, int utf8);

#endif /* KFONT_CONTEXTP_H */
