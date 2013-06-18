#ifndef LK_KERNEL_H
#define LK_KERNEL_H

#include <keymap/context.h>

int lk_kernel_keymap(struct lk_ctx *ctx, int console);
int lk_kernel_keys(struct lk_ctx *ctx, int console);
int lk_kernel_funcs(struct lk_ctx *ctx, int console);
int lk_kernel_diacrs(struct lk_ctx *ctx, int console);

#endif /* LK_KERNEL_H */
