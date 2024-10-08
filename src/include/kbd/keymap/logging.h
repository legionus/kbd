// SPDX-License-Identifier: LGPL-2.0-or-later
/**
 * @file logging.h
 * @brief Functions for logging.
 */
#ifndef _KBD_LIBKEYMAP_LOGGING_H_
#define _KBD_LIBKEYMAP_LOGGING_H_

#include <syslog.h>

#include <kbd/compiler_attributes.h>

#include <kbd/keymap/context.h>

typedef void (*lk_logger_t)(void *, int, const char *, int, const char *, const char *, va_list)
	KBD_ATTR_PRINTF(6, 0)
	KBD_ATTR_NONNULL(1);

/**
 * Logging function which uses @ref lk_ctx::log_fn "log_fn" and
 * @ref lk_ctx::log_data "log_data" to write log messages.
 * @param ctx is a keymap library context.
 * @param priority indicates the priority.
 */
void
lk_log(struct lk_ctx *ctx, int priority,
		const char *file, int line, const char *fn,
		const char *fmt, ...)
	KBD_ATTR_PRINTF(6, 7)
	KBD_ATTR_NONNULL(1);

#endif /* _KBD_LIBKEYMAP_LOGGING_H_ */
