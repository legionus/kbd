// SPDX-License-Identifier: LGPL-2.0-or-later
/**
 * @file features.h
 * @brief Kernel keymap feature detection.
 */
#ifndef _KBD_LIBKEYMAP_FEATURES_H_
#define _KBD_LIBKEYMAP_FEATURES_H_

#include <kbd/compiler_attributes.h>

struct lk_ctx;

/** Features known to libkeymap. */
enum lk_feature {
	LK_FEATURE_KT_CSI,
	LK_FEATURE_COUNT,
};

/** Result of probing a keymap feature. */
enum lk_feature_status {
	LK_FEATURE_UNCHECKED,
	LK_FEATURE_SUPPORTED,
	LK_FEATURE_UNSUPPORTED,
	LK_FEATURE_UNAVAILABLE,
};

/**
 * Probe every feature known to libkeymap on @p console_fd.
 *
 * A feature is unavailable when its probe cannot complete reliably. In that
 * case this function returns -1 and callers must not treat the feature as
 * unsupported. Some probes temporarily change the keyboard mode in order to
 * distinguish a supported action type from an unknown Unicode value; the
 * original mode is restored and the temporary keymap table is removed before
 * this function returns.
 */
int lk_check_features(struct lk_ctx *ctx, int console_fd)
	KBD_ATTR_NONNULL(1);

/** Return the current status of @p feature in @p ctx. */
enum lk_feature_status
lk_get_feature_status(const struct lk_ctx *ctx, enum lk_feature feature)
	KBD_ATTR_NONNULL(1);

/** Resolve a feature name used by a keymap, or return LK_FEATURE_COUNT. */
enum lk_feature lk_feature_from_name(const char *name)
	KBD_ATTR_NONNULL(1);

/** Return the stable keymap name for @p feature, or NULL if it is invalid. */
const char *lk_feature_name(enum lk_feature feature);

#endif /* _KBD_LIBKEYMAP_FEATURES_H_ */
