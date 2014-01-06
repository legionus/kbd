/**
 * @file kmap.h
 * @brief Functions for keymaps manipulation (add/delete keys).
 */
#ifndef LK_KMAP_H
#define LK_KMAP_H

#include <keymap/context.h>
#include <keymap/findfile.h>

int lk_add_map(struct lk_ctx *ctx,   unsigned int k_table);
int lk_map_exists(struct lk_ctx *ctx, unsigned int k_table);

int lk_maps_total(struct lk_ctx *ctx);
int lk_keys_total(struct lk_ctx *ctx, unsigned int k_table);

int lk_add_key(struct lk_ctx *ctx, unsigned int k_table, unsigned int k_index, int keycode);
int lk_del_key(struct lk_ctx *ctx, unsigned int k_table, unsigned int k_index);
int lk_get_key(struct lk_ctx *ctx, unsigned int k_table, unsigned int k_index);
int lk_key_exists(struct lk_ctx *ctx,   unsigned int k_table, unsigned int k_index);

int lk_get_func(struct lk_ctx *ctx, struct kbsentry *kbs);
int lk_add_func(struct lk_ctx *ctx, struct kbsentry kbs);

int lk_add_diacr(struct lk_ctx *ctx, unsigned int diacr, unsigned int base, unsigned int res);
int lk_add_compose(struct lk_ctx *ctx, unsigned int diacr, unsigned int base, unsigned int res);

int lk_add_constants(struct lk_ctx *ctx);

int lk_parse_keymap(struct lk_ctx *ctx, lkfile_t *f);
int lk_load_keymap(struct lk_ctx *ctx, int fd, int kbd_mode);

#endif /* LK_KMAP_H */
