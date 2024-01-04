#ifndef _XKB_SUPPORT_H_
#define _XKB_SUPPORT_H_

#include <xkbcommon/xkbcommon.h>
#include "keymap.h"

struct xkeymap_params {
	const char *model;
	const char *layout;
	const char *variant;
	const char *options;
};

int convert_xkb_keymap(struct lk_ctx *ctx, struct xkeymap_params *params);

#endif /* _XKB_SUPPORT_H_ */
