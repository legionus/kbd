#ifndef LK_DATA_H
#define LK_DATA_H

#include <linux/kd.h>
#include <linux/keyboard.h>
#include <keymap/findfile.h>
#include <keymap/array.h>

/**
 * @brief Copy of struct kbdiacruc.
 */
struct kb_diacr {
	unsigned int diacr, base, result;
};

/**
 * @brief Parser flags that are set outside the library.
 */
typedef enum {
	LK_FLAG_UNICODE_MODE   = (1 << 1), /**< Unicode mode */
	LK_FLAG_CLEAR_COMPOSE  = (1 << 2), /**< Compose */
	LK_FLAG_CLEAR_STRINGS  = (1 << 3), /**< Strings */
	LK_FLAG_PREFER_UNICODE = (1 << 4)  /**< Prefer unicode */
} lk_flags;

/**
 * @brief Keywords used in keymap files.
 */
typedef enum {
	LK_KEYWORD_KEYMAPS    = (1 << 1), /**< 'Keymaps' keyword */
	LK_KEYWORD_ALTISMETA  = (1 << 2), /**< 'Alt-is-meta' keyword */
	LK_KEYWORD_CHARSET    = (1 << 3), /**< 'Charset' keyword */
	LK_KEYWORD_STRASUSUAL = (1 << 4)  /**< 'String as usual' keyword */
} lk_keywords;

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

	/** @protected
	 * User defined logging function.
	 */
	void (*log_fn)(void *data, int priority,
	               const char *file, int line, const char *fn,
	               const char *format, va_list args);

	/** @protected
	 * The data passed to the @ref log_fn logging function as the first argument.
	 */
	void *log_data;

	/** @protected
	 * Logging priority used by @ref log_fn logging function.
	 */
	int log_priority;

	/** @protected
	 * User defined charset.
	 */
	unsigned int charset;

	/* Fields used by keymap parser */

	struct lk_array *key_constant;      /**< @private */
	struct lk_array *key_line;          /**< @private */
	int mod;                            /**< @private */
	lkfile_t *stack[MAX_INCLUDE_DEPTH]; /**< @private */
};

#endif /* LK_DATA_H */
