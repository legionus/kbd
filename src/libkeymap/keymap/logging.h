/**
 * @file logging.h
 * @brief Functions for logging.
 */
#ifndef LK_LOGGING_H
#define LK_LOGGING_H

#include <syslog.h>
#include <keymap/context.h>

/**
 * Logging function which uses @ref lk_ctx::log_fn "log_fn" and
 * @ref lk_ctx::log_data "log_data" to write log messages.
 * @param ctx is a keymap library context.
 * @param priority indicates the priority.
 */
void lk_log(struct lk_ctx *ctx, int priority,
            const char *file, int line, const char *fn,
            const char *fmt, ...);

#define lk_log_cond(ctx, level, arg...) \
	do { \
		if (ctx->log_priority >= level) \
			lk_log(ctx, level, __FILE__, __LINE__, __func__, ## arg);\
	} while (0)

/**
 * Wrapper to output debug-level messages
 * @param ctx is a keymap library context.
 * @param arg is output message.
 */
#define DBG(ctx, arg...)  lk_log_cond(ctx, LOG_DEBUG,   ## arg)

/**
 * Wrapper to output informational messages
 * @param ctx is a keymap library context.
 * @param arg is output message.
 */
#define INFO(ctx, arg...) lk_log_cond(ctx, LOG_INFO,    ## arg)

/**
 * Wrapper to output warning conditions
 * @param ctx is a keymap library context.
 * @param arg is output message.
 */
#define WARN(ctx, arg...) lk_log_cond(ctx, LOG_WARNING, ## arg)

/**
 * Wrapper to output error conditions
 * @param ctx is a keymap library context.
 * @param arg is output message.
 */
#define ERR(ctx, arg...)  lk_log_cond(ctx, LOG_ERR,     ## arg)

#endif /* LK_LOGGING_H */
