#ifndef _XKB_SUPPORT_H_
#define _XKB_SUPPORT_H_

#include <xkbcommon/xkbcommon.h>
#include "keymap.h"

int convert_xkb_keymap(struct lk_ctx *ctx, struct xkb_rule_names *names, int options);

#endif /* _XKB_SUPPORT_H_ */
