// SPDX-License-Identifier: LGPL-2.0-or-later
/**
 * @file kernel.h
 * @brief Functions for loading objects into the kernel.
 */
#ifndef _KBD_LIBKEYMAP_KERNEL_H_
#define _KBD_LIBKEYMAP_KERNEL_H_

#include <kbd/compiler_attributes.h>

#include <kbd/keymap/context.h>

/**
 * Loads keymap into the kernel. This is a high-level function that calls
 * @ref lk_kernel_keys, @ref lk_kernel_funcs and @ref lk_kernel_diacrs.
 * @param ctx is a keymap library context.
 * @param console is open file descriptor.
 *
 * @return 0 on success, -1 on error.
 */
int lk_kernel_keymap(struct lk_ctx *ctx, int console)
	KBD_ATTR_NONNULL(1);

/**
 * Loads keycodes into the kernel.
 * @param ctx is a keymap library context.
 * @param console is open file descriptor.
 *
 * @return 0 on success, -1 on error.
 */
int lk_kernel_keys(struct lk_ctx *ctx, int console)
	KBD_ATTR_NONNULL(1);

/**
 * Loads function keys into the kernel.
 * @param ctx is a keymap library context.
 * @param console is open file descriptor.
 *
 * @return 0 on success, -1 on error.
 */
int lk_kernel_funcs(struct lk_ctx *ctx, int console)
	KBD_ATTR_NONNULL(1);

/**
 * Loads accent table into the kernel.
 * @param ctx is a keymap library context.
 * @param console is open file descriptor.
 *
 * @return 0 on success, -1 on error.
 */
int lk_kernel_diacrs(struct lk_ctx *ctx, int console)
	KBD_ATTR_NONNULL(1);

#endif /* _KBD_LIBKEYMAP_KERNEL_H_ */
