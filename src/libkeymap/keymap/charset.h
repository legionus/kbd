/**
 * @file charset.h
 * @brief Functions for charset manipulation.
 */
#ifndef LK_CHARSET_H
#define LK_CHARSET_H

#include <keymap/context.h>

void lk_list_charsets(FILE *f);
const char *lk_get_charset(struct lk_ctx *ctx);
int lk_set_charset(struct lk_ctx *ctx, const char *name);

#endif /* LK_CHARSET_H */
