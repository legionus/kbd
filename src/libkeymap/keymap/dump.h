#ifndef LK_DUMP_H
#define LK_DUMP_H

#include <stdio.h>

#include <keymap/context.h>

int lk_dump_bkeymap(struct lk_ctx *ctx, FILE *fd);
int lk_dump_ctable(struct lk_ctx *ctx, FILE *fd);

void lk_dump_keymap(struct lk_ctx *ctx, FILE *fd, char table_shape, char numeric);
void lk_dump_keymaps(struct lk_ctx *ctx, FILE *fd);
void lk_dump_funcs(struct lk_ctx *ctx, FILE *fd);
void lk_dump_diacs(struct lk_ctx *ctx, FILE *fd);

#define DEFAULT         0
#define FULL_TABLE      1 /* one line for each keycode */
#define SEPARATE_LINES  2 /* one line for each (modifier,keycode) pair */
#define	UNTIL_HOLE      3 /* one line for each keycode, until 1st hole */

void lk_dump_keys(struct lk_ctx *ctx, FILE *fd, char table_shape, char numeric);

void lk_dump_summary(struct lk_ctx *ctx, FILE *fd, int console);
void lk_dump_symbols(FILE *fd);

#endif /* LK_DUMP_H */
