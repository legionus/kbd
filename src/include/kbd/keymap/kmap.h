// SPDX-License-Identifier: LGPL-2.0-or-later
/**
 * @file kmap.h
 * @brief Functions for keymaps manipulation (add/delete keys).
 */
#ifndef _KBD_LIBKEYMAP_KMAP_H_
#define _KBD_LIBKEYMAP_KMAP_H_

#include <kbd/compiler_attributes.h>

#include <kbd/keymap/context.h>

int lk_add_map(struct lk_ctx *ctx, int k_table)
	KBD_ATTR_NONNULL(1);

int lk_map_exists(struct lk_ctx *ctx, int k_table)
	KBD_ATTR_NONNULL(1);

int lk_get_keys_total(struct lk_ctx *ctx, int k_table)
	KBD_ATTR_NONNULL(1);

int lk_add_key(struct lk_ctx *ctx, int k_table, int k_index, int keycode)
	KBD_ATTR_NONNULL(1);

int lk_del_key(struct lk_ctx *ctx, int k_table, int k_index)
	KBD_ATTR_NONNULL(1);

int lk_get_key(struct lk_ctx *ctx, int k_table, int k_index)
	KBD_ATTR_NONNULL(1);

int lk_key_exists(struct lk_ctx *ctx, int k_table, int k_index)
	KBD_ATTR_NONNULL(1);

/* Functions for key string manipulations */
int lk_get_func(struct lk_ctx *ctx, struct kbsentry *kbs)
	KBD_ATTR_NONNULL(1, 2);

int lk_add_func(struct lk_ctx *ctx, struct kbsentry *kbs)
	KBD_ATTR_NONNULL(1, 2);

int lk_del_func(struct lk_ctx *ctx, int index)
	KBD_ATTR_NONNULL(1);

int lk_func_exists(struct lk_ctx *ctx, int index)
	KBD_ATTR_NONNULL(1);

/* Functions for manipulations with diacritical table */
int lk_get_diacr(struct lk_ctx *ctx, int index, struct lk_kbdiacr *dcr)
	KBD_ATTR_NONNULL(1, 3);

int lk_add_diacr(struct lk_ctx *ctx, int index, struct lk_kbdiacr *dcr)
	KBD_ATTR_NONNULL(1, 3);

int lk_del_diacr(struct lk_ctx *ctx, int index)
	KBD_ATTR_NONNULL(1);

int lk_diacr_exists(struct lk_ctx *ctx, int index)
	KBD_ATTR_NONNULL(1);

int lk_append_diacr(struct lk_ctx *ctx, struct lk_kbdiacr *dcr)
	KBD_ATTR_NONNULL(1, 2);

int lk_append_compose(struct lk_ctx *ctx, struct lk_kbdiacr *dcr)
	KBD_ATTR_NONNULL(1, 2);

int lk_add_constants(struct lk_ctx *ctx) KBD_ATTR_NONNULL(1);

#include <kbdfile.h>

int lk_parse_keymap(struct lk_ctx *ctx, struct kbdfile *f)
	KBD_ATTR_NONNULL(1, 2);

int lk_load_keymap(struct lk_ctx *ctx, int fd, int kbd_mode)
	KBD_ATTR_NONNULL(1);

#endif /* _KBD_LIBKEYMAP_KMAP_H_ */
