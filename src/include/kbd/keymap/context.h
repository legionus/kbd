// SPDX-License-Identifier: LGPL-2.0-or-later
/**
 * @file context.h
 * @brief Header contains flags, keywords and context structure.
 */
#ifndef _KBD_LIBKEYMAP_CONTEXT_H_
#define _KBD_LIBKEYMAP_CONTEXT_H_

#include <linux/kd.h>
#include <linux/keyboard.h>

#include <kbd/compiler_attributes.h>

#include <kbd/keymap/array.h>

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
 * @brief Copy of struct kbdiacruc.
 */
struct lk_kbdiacr {
	unsigned int diacr, base, result;
};

/**
 * @brief Opaque object representing the library context.
 */
struct lk_ctx;

/* Returned by ksymtocode to report an unknown symbol */
#define CODE_FOR_UNKNOWN_KSYM (-1)

/* Directions for converting keysyms */
#define TO_AUTO    (-1) /* use LK_FLAG_PREFER_UNICODE */
#define TO_8BIT    0
#define TO_UNICODE 1

int lk_convert_code(struct lk_ctx *ctx, int code, int direction);
int lk_add_capslock(struct lk_ctx *ctx, int code);

#endif /* _KBD_LIBKEYMAP_CONTEXT_H_ */
