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

#endif /* KFONT_CONTEXTP_H */
