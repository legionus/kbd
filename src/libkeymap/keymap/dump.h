#ifndef LK_DUMP_H
#define LK_DUMP_H

#include <stdio.h>

#include <keymap/context.h>

/**
 * @brief Flags controlling the output keymap.
 */
typedef enum {
	LK_SHAPE_DEFAULT        = (1 << 1),
	LK_SHAPE_FULL_TABLE     = (1 << 2), /**< one line for each keycode */
	LK_SHAPE_SEPARATE_LINES = (1 << 3), /**< one line for each (modifier,keycode) pair */
	LK_SHAPE_UNTIL_HOLE     = (1 << 4)  /**< one line for each keycode until 1st hole */
} lk_table_shape;

/**
 * Outputs a keymap in binary format.
 * @param ctx is a keymap library context.
 * @param fd is a FILE pointer for output.
 *
 * @return 0 on success, -1 on error.
 */
int lk_dump_bkeymap(struct lk_ctx *ctx, FILE *fd);

/**
 * Outputs a keymap in C format.
 * @param ctx is a keymap library context.
 * @param fd is a FILE pointer for output.
 *
 * @return 0 on success, -1 on error.
 */
int lk_dump_ctable(struct lk_ctx *ctx, FILE *fd);

/**
 * Outputs whole keymap. This is a high-level function that calls @ref lk_dump_keys,
 * @ref lk_dump_keymaps, @ref lk_dump_funcs and @ref lk_dump_diacs.
 * @param ctx is a keymap library context.
 * @param fd is a FILE pointer for output.
 * @param table specifies the output format of the keycode table.
 * @param numeric indicates whether to output the keycodes in numerical form.
 */
void lk_dump_keymap(struct lk_ctx *ctx, FILE *fd, lk_table_shape table, char numeric);

/**
 * Outputs keycodes.
 * @param ctx is a keymap library context.
 * @param fd is a FILE pointer for output.
 * @param table specifies the output format of the keycode table.
 * @param numeric indicates whether to output the keycodes in numerical form.
 */
void lk_dump_keys(struct lk_ctx *ctx, FILE *fd, lk_table_shape table, char numeric);

/**
 * Outputs 'keymaps' line.
 * @param ctx is a keymap library context.
 * @param fd is a FILE pointer for output.
 */
void lk_dump_keymaps(struct lk_ctx *ctx, FILE *fd);

/**
 * Outputs function keys.
 * @param ctx is a keymap library context.
 * @param fd is a FILE pointer for output.
 */
void lk_dump_funcs(struct lk_ctx *ctx, FILE *fd);

/**
 * Outputs accent table.
 * @param ctx is a keymap library context.
 * @param fd is a FILE pointer for output.
 */
void lk_dump_diacs(struct lk_ctx *ctx, FILE *fd);

void lk_dump_summary(struct lk_ctx *ctx, FILE *fd, int console);
void lk_dump_symbols(FILE *fd);

#endif /* LK_DUMP_H */
