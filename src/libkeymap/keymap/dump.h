#ifndef LK_DUMP_H
#define LK_DUMP_H

#include <stdio.h>

#include <keymap/context.h>

typedef enum {
	LK_SHAPE_DEFAULT        = (1 << 1),
	LK_SHAPE_FULL_TABLE     = (1 << 2), /* one line for each keycode */
	LK_SHAPE_SEPARATE_LINES = (1 << 3), /* one line for each (modifier,keycode) pair */
	LK_SHAPE_UNTIL_HOLE     = (1 << 4)  /* one line for each keycode, until 1st hole */
} lk_table_shape;

int lk_dump_bkeymap(struct lk_ctx *ctx, FILE *fd);
int lk_dump_ctable(struct lk_ctx *ctx, FILE *fd);

void lk_dump_keymap(struct lk_ctx *ctx, FILE *fd, lk_table_shape table, char numeric);
void lk_dump_keys(struct lk_ctx *ctx, FILE *fd, lk_table_shape table, char numeric);
void lk_dump_keymaps(struct lk_ctx *ctx, FILE *fd);
void lk_dump_funcs(struct lk_ctx *ctx, FILE *fd);
void lk_dump_diacs(struct lk_ctx *ctx, FILE *fd);

void lk_dump_summary(struct lk_ctx *ctx, FILE *fd, int console);
void lk_dump_symbols(FILE *fd);

#endif /* LK_DUMP_H */
