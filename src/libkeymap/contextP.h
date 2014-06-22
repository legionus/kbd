#ifndef LK_CONTEXTP_H
#define LK_CONTEXTP_H

#include <stdarg.h>

#include "keymap.h"

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
	 * User defined logging function.
	 */
	void (*log_fn)(void *data, int priority,
	               const char *file, int line, const char *fn,
	               const char *format, va_list args);

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
	unsigned int charset;

	/* Fields used by keymap parser */

	struct lk_array *key_constant;
	struct lk_array *key_line;
	int mod;
	lkfile_t *stack[MAX_INCLUDE_DEPTH];
};

#endif /* LK_CONTEXTP_H */
