#ifndef LK_CONTEXTP_H
#define LK_CONTEXTP_H

#include <stdarg.h>
#include <kbdfile.h>

/*
 * NLS -- the library has to be independent on main program, so define
 * KBD_TEXTDOMAIN_EXPLICIT before you include nls.h.
 *
 * Now we use kbd.po (=PACKAGE), rather than maintain the texts
 * in the separate libkeymap.po file.
 */
#define LIBKEYMAP_TEXTDOMAIN	PACKAGE
#define KBD_TEXTDOMAIN_EXPLICIT	LIBKEYMAP_TEXTDOMAIN
#include "nls.h"

#include "keymap.h"

/**
 * @brief The first 128 code points of Unicode are the same as ASCII.
 */
#define UNICODE_ASCII_LEN 128

#define UNICODE_MASK 0xf000
#define U(x) ((x) ^ UNICODE_MASK)

#define BIT_32(nr)		((1) << (nr))

#define GENMASK_32(h, l) \
	(((~(0)) - ((1) << (l)) + 1) & (~(0) >> (31 - (h))))

/**
 * @brief Bits above the maximum unicode (0x10ffff) available for flags.
 */
#define FLAG_MASK		GENMASK_32(31, 24)
#define FLAG_UNICODE		BIT_32(30)

#define STRIP_FLAGS(x)		((x) & ~FLAG_MASK)

#define KBD_KVALUE(x)		((x))
#define KBD_UNICODE(x)		((x) | FLAG_UNICODE)

#define KBD_KTYP(x)		KTYP(STRIP_FLAGS(x))
#define KBD_KVAL(x)		KVAL(STRIP_FLAGS(x))

static inline unsigned int export_value(unsigned int value)
{
	if ((value & FLAG_UNICODE))
		value = U(value);

	return STRIP_FLAGS(value);
}

/**
 * @brief The maximum number of include levels.
 */
#define MAX_INCLUDE_DEPTH 20

/**
 * @brief Opaque object representing the library context.
 */
struct lk_ctx {
	/**
	 * Parser flags that are set outside the library.
	 */
	lk_flags flags;

	/**
	 * Keywords used in keymap files.
	 */
	lk_keywords keywords;

	/**
	 * Key translation table (keycode to action code).
	 */
	struct lk_array *keymap;

	/**
	 * Function key string entry.
	 */
	struct lk_array *func_table;

	/**
	 * Accent table.
	 */
	struct lk_array *accent_table;

	/**
	 * Context for libkbdfile.
	 */
	struct kbdfile_ctx *kbdfile_ctx;

	/**
	 * User defined logging function.
	 */
	lk_logger_t log_fn;

	/**
	 * The data passed to the @ref log_fn logging function as the first argument.
	 */
	void *log_data;

	/**
	 * Logging priority used by @ref log_fn logging function.
	 */
	int log_priority;

	/**
	 * User defined charset.
	 */
	unsigned short charset;

	/* Fields used by keymap parser */

	struct lk_array *key_constant;
	struct lk_array *key_line;
	int mod;
	struct kbdfile *stack[MAX_INCLUDE_DEPTH];
};

#define lk_log_cond(ctx, level, arg...)                                          \
	do {                                                                     \
		if (ctx->log_priority >= level)                                  \
			lk_log(ctx, level, __FILE__, __LINE__, __func__, ##arg); \
	} while (0)

/**
 * Wrapper to output debug-level messages
 * @param ctx is a keymap library context.
 * @param arg is output message.
 */
#define DBG(ctx, arg...) lk_log_cond(ctx, LOG_DEBUG, ##arg)

/**
 * Wrapper to output informational messages
 * @param ctx is a keymap library context.
 * @param arg is output message.
 */
#define INFO(ctx, arg...) lk_log_cond(ctx, LOG_INFO, ##arg)

/**
 * Wrapper to output warning conditions
 * @param ctx is a keymap library context.
 * @param arg is output message.
 */
#define WARN(ctx, arg...) lk_log_cond(ctx, LOG_WARNING, ##arg)

/**
 * Wrapper to output error conditions
 * @param ctx is a keymap library context.
 * @param arg is output message.
 */
#define ERR(ctx, arg...) lk_log_cond(ctx, LOG_ERR, ##arg)

#endif /* LK_CONTEXTP_H */
