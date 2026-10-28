#ifndef _MODIFIERS_H
#define _MODIFIERS_H

#include <linux/keyboard.h>

#define M_PLAIN 0U
#define M_SHIFT (1U << KG_SHIFT)
#define M_CTRL (1U << KG_CTRL)
#define M_ALT (1U << KG_ALT)
#define M_ALTGR (1U << KG_ALTGR)
#define M_SHIFTL (1U << KG_SHIFTL)
#define M_SHIFTR (1U << KG_SHIFTR)
#define M_CTRLL (1U << KG_CTRLL)
#define M_CTRLR (1U << KG_CTRLR)
#define M_CAPSSHIFT (1U << KG_CAPSSHIFT)

typedef struct {
	const char *name;
	const int bit;
} modifier_t;

extern const modifier_t modifiers[];

#endif /* _MODIFIERS_H */
