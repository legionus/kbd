/**
 * @file logging.h
 * @brief Functions for logging.
 */
#ifndef LK_LOGGING_H
#define LK_LOGGING_H

#include <syslog.h>
#include <keymap/context.h>

#ifndef __GNUC__
#define __attribute__(x) /*NOTHING*/
#endif

typedef void (*lk_logger_t)(void *, int, const char *, int, const char *, const char *, va_list) __attribute__((format(printf, 6, 0)));

/**
 * Logging function which uses @ref lk_ctx::log_fn "log_fn" and
 * @ref lk_ctx::log_data "log_data" to write log messages.
 * @param ctx is a keymap library context.
 * @param priority indicates the priority.
 */
void
__attribute__((format(printf, 6, 7)))
lk_log(struct lk_ctx *ctx, int priority,
       const char *file, int line, const char *fn,
       const char *fmt, ...);

#endif /* LK_LOGGING_H */
