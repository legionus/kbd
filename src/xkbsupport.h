#ifndef _XKB_SUPPORT_H_
#define _XKB_SUPPORT_H_

#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-compose.h>
#include "keymap.h"

struct xkeymap_params {
	const char *model;
	const char *layout;
	const char *variant;
	const char *options;
	const char *locale;
};

int convert_xkb_keymap(struct lk_ctx *ctx, struct xkeymap_params *params, int options);

#endif /* _XKB_SUPPORT_H_ */
