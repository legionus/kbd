/**
 * @file kernel.h
 * @brief Functions for loading objects into the kernel.
 */
#ifndef LK_KERNEL_H
#define LK_KERNEL_H

#include <keymap/context.h>

/**
 * Loads keymap into the kernel. This is a high-level function that calls
 * @ref lk_kernel_keys, @ref lk_kernel_funcs and @ref lk_kernel_diacrs.
 * @param ctx is a keymap library context.
 * @param console is open file descriptor.
 *
 * @return 0 on success, -1 on error.
 */
int lk_kernel_keymap(struct lk_ctx *ctx, int console);

/**
 * Loads keycodes into the kernel.
 * @param ctx is a keymap library context.
 * @param console is open file descriptor.
 *
 * @return 0 on success, -1 on error.
 */
int lk_kernel_keys(struct lk_ctx *ctx, int console);

/**
 * Loads function keys into the kernel.
 * @param ctx is a keymap library context.
 * @param console is open file descriptor.
 *
 * @return 0 on success, -1 on error.
 */
int lk_kernel_funcs(struct lk_ctx *ctx, int console);

/**
 * Loads accent table into the kernel.
 * @param ctx is a keymap library context.
 * @param console is open file descriptor.
 *
 * @return 0 on success, -1 on error.
 */
int lk_kernel_diacrs(struct lk_ctx *ctx, int console);

#endif /* LK_KERNEL_H */
