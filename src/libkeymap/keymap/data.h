#ifndef LK_DATA_H
#define LK_DATA_H

#include <linux/kd.h>
#include <linux/keyboard.h>
#include <keymap/findfile.h>
#include <keymap/array.h>

struct kb_diacr {
	unsigned int diacr, base, result;
};

typedef enum {
	LK_FLAG_UNICODE_MODE  = (1 << 1),
	LK_FLAG_CLEAR_COMPOSE = (1 << 2),
	LK_FLAG_CLEAR_STRINGS = (1 << 3),
	LK_FLAG_PREFER_UNICODE= (1 << 4)
} lk_flags;

typedef enum {
	LK_KEYWORD_KEYMAPS    = (1 << 1),
	LK_KEYWORD_ALTISMETA  = (1 << 2),
	LK_KEYWORD_CHARSET    = (1 << 3),
	LK_KEYWORD_STRASUSUAL = (1 << 4)
} lk_keywords;

#define MAX_INCLUDE_DEPTH 20

struct keymap {
	/* Parser flags */
	lk_flags flags;

	/* Keymap keywords (keymaps, alt-is-meta, charset, ...) */
	lk_keywords keywords;

	struct lk_array *keymap;

	/* the kernel structures we want to set or print */
	struct lk_array *func_table;

	struct lk_array *accent_table;
	struct lk_array *key_constant;

	/* Key definitions on one line */
	struct lk_array *key_line;

	/* Line by line modifiers */
	int mod;

	unsigned int charset;

	lkfile_t *stack[MAX_INCLUDE_DEPTH];

	int log_priority;
	void (*log_fn)(void *data, int priority,
	               const char *file, int line, const char *fn,
	               const char *format, va_list args);
	void *log_data;
};

#endif /* LK_DATA_H */
