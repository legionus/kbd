#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <search.h>

#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include "libcommon.h"
#include "keymap.h"
#include "xkbsupport.h"

#include <xkbcommon/xkbregistry.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

static int
xkeymap_warnings_suppressed(void)
{
	const char *env = getenv("LK_XKB_SUPPRESS_WARNINGS");

	return env && env[0] != '\0' && strcmp(env, "0") != 0;
}

#define XKEYMAP_WARNING(...) \
	do { \
		if (!xkeymap_warnings_suppressed()) \
			kbd_warning(__VA_ARGS__); \
	} while (0)

/*
 * The number of layouts that can be processed by this implementation.
 */
#define NR_LAYOUTS 4

static const unsigned int layout_switch[NR_LAYOUTS] = {
	(0),
	(1U << KG_SHIFTL),
	(1U << KG_SHIFTR),
	(1U << KG_SHIFTL) | (1U << KG_SHIFTR)
};

/*
 * number    |
 * of groups | group
 * --------------------------
 *         0 | { 0, 0, 0, 0 }
 *         1 | { 0, 1, 1, 0 }
 *         2 | { 0, 1, 2, 0 }
 *         3 | { 0, 1, 3, 2 }
 */
static const unsigned int layouts[NR_LAYOUTS][NR_LAYOUTS] = {
	{ 0, 0, 0, 0 },
	{ 0, 1, 1, 0 },
	{ 0, 1, 2, 0 },
	{ 0, 1, 3, 2 },
};

struct reachable_sym {
	xkb_keysym_t sym;
	int code;
};

enum xkeymap_modifier_mask {
	XKEYMAP_MASK_SHIFT,
	XKEYMAP_MASK_CONTROL,
	XKEYMAP_MASK_ALT,
	XKEYMAP_MASK_LEVEL3,
	XKEYMAP_MASK__COUNT,
};

struct xkeymap {
	struct xkb_context *xkb;
	struct xkb_keymap *keymap;
	struct xkb_compose_table *compose;
	struct lk_ctx *ctx;
	void *reachable_syms;
	xkb_mod_mask_t modifier_masks[XKEYMAP_MASK__COUNT];
};

struct compose_candidate {
	struct lk_kbdiacr diacr;
	xkb_keysym_t seq[2];
	xkb_keysym_t result_sym;
	int result_reachable;
	int score;
};

/*
 * Offset between evdev keycodes (where KEY_ESCAPE is 1), and the evdev XKB
 * keycode set (where ESC is 9).
 */
#define EVDEV_OFFSET 8

/*
 * Convert xkb keycode to kernel keycode.
 */
#define KERN_KEYCODE(keycode) (keycode - EVDEV_OFFSET)

static xkb_mod_mask_t xkb_mod_bit(xkb_mod_index_t mod);
static unsigned int xkb_mask_weight(xkb_mod_mask_t mask);
static int xkeymap_get_symbol(struct xkb_keymap *keymap,
		xkb_keycode_t keycode, xkb_layout_index_t layout, xkb_level_index_t level,
		xkb_keysym_t *symbol);

struct xkeymap_modifier_rule {
	enum xkeymap_modifier_mask mask;
	const char *name;
	xkb_keysym_t keysym;
	unsigned int kernel_bit;
};

static const struct xkeymap_modifier_rule xkeymap_modifier_rules[] = {
	{ XKEYMAP_MASK_SHIFT,		"Shift",		0,				(1u << KG_SHIFT) },
	{ XKEYMAP_MASK_CONTROL,		"Control",		0,				(1u << KG_CTRL) },
	{ XKEYMAP_MASK_ALT,		NULL,			XKB_KEY_Alt_L,			(1u << KG_ALT) },
	{ XKEYMAP_MASK_ALT,		NULL,			XKB_KEY_Alt_R,			(1u << KG_ALT) },
	{ XKEYMAP_MASK_LEVEL3,		NULL,			XKB_KEY_ISO_Level3_Shift,	(1u << KG_ALTGR) },
};

static xkb_mod_mask_t xkeymap_mod_mask_by_name(struct xkb_keymap *keymap, const char *name)
{
	xkb_mod_index_t idx = xkb_keymap_mod_get_index(keymap, name);

	if (idx == XKB_MOD_INVALID)
		return 0;

	return xkb_mod_bit(idx);
}

/*
 * libxkbcommon exposes named real modifiers directly, but it does not have a
 * direct API to ask which real modifier mask implements a semantic keysym like
 * ISO_Level3_Shift in the compiled keymap.  Derive that mask indirectly by
 * probing candidate keys through xkb_state; this is necessarily a small
 * workaround rather than a first-class query.
 */
static xkb_mod_mask_t xkeymap_find_modifier_mask_for_keysym(struct xkeymap *xkeymap,
							    xkb_keysym_t target)
{
	struct xkb_state *state;
	xkb_keycode_t min, max;
	xkb_mod_mask_t best = 0;
	unsigned int best_weight = UINT_MAX;

	state = xkb_state_new(xkeymap->keymap);
	if (!state)
		return 0;

	min = xkb_keymap_min_keycode(xkeymap->keymap);
	max = xkb_keymap_max_keycode(xkeymap->keymap);

	/*
	 * Semantic modifier keysyms are not guaranteed to live on layout 0.
	 * Some rulesets expose the effective LevelThree/LevelFive chooser only
	 * in another group, so probing layout 0 alone misses a valid semantic
	 * modifier and later makes reachable XKB levels look unrepresentable in
	 * the kernel tables.
	 *
	 * Probe level 0 of every layout by first selecting that layout in the
	 * temporary xkb_state and then pressing the candidate key. We
	 * intentionally do not try to infer modifier-dependent higher levels
	 * here: once another modifier is needed just to reach the keysym, a
	 * blind synthetic press no longer tells us which part of the resulting
	 * state belongs to the probed modifier and which part came from the
	 * setup needed to reach that level.
	 */
	for (xkb_keycode_t keycode = min; keycode <= max; keycode++) {
		xkb_layout_index_t num_layouts = xkb_keymap_num_layouts_for_key(xkeymap->keymap, keycode);

		for (xkb_layout_index_t layout = 0; layout < num_layouts; layout++) {
			xkb_keysym_t sym;
			int ret;

			ret = xkeymap_get_symbol(xkeymap->keymap, keycode, layout, 0, &sym);
			if (ret <= 0 || sym != target)
				continue;

			xkb_state_update_mask(state, 0, 0, 0, 0, 0, layout);
			xkb_state_update_key(state, keycode, XKB_KEY_DOWN);

			ret = (int) xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE);

			xkb_state_update_key(state, keycode, XKB_KEY_UP);
			xkb_state_update_mask(state, 0, 0, 0, 0, 0, 0);

			if (ret <= 0)
				continue;

			xkb_mod_mask_t candidate = (xkb_mod_mask_t) ret;
			unsigned int candidate_weight = xkb_mask_weight(candidate);

			if (!best || candidate_weight < best_weight ||
			    (candidate_weight == best_weight && candidate < best)) {
				best = candidate;
				best_weight = candidate_weight;
			}
		}
	}

	xkb_state_unref(state);
	return best;
}

static void xkeymap_init_modifier_masks(struct xkeymap *xkeymap)
{
	memset(xkeymap->modifier_masks, 0, sizeof(xkeymap->modifier_masks));

	/*
	 * xkb_state_update_mask() takes real modifier masks. XKB rulesets may
	 * bind Alt and LevelThree to different ModN bits, so derive their masks
	 * from the compiled keymap instead of assuming fixed assignments.
	 */
	for (size_t i = 0; i < ARRAY_SIZE(xkeymap_modifier_rules); i++) {
		const struct xkeymap_modifier_rule *rule = &xkeymap_modifier_rules[i];
		xkb_mod_mask_t *mask = &xkeymap->modifier_masks[rule->mask];

		if (*mask)
			continue;

		if (rule->name)
			*mask = xkeymap_mod_mask_by_name(xkeymap->keymap, rule->name);
		else
			*mask = xkeymap_find_modifier_mask_for_keysym(xkeymap, rule->keysym);
	}
}

static int compare_compose_order(unsigned int lhs, unsigned int rhs)
{
	if (lhs < rhs)
		return -1;
	if (lhs > rhs)
		return 1;

	return 0;
}

static int compare_reachable_syms(const void *pa, const void *pb)
{
	const struct reachable_sym *lhs = pa;
	const struct reachable_sym *rhs = pb;

	return compare_compose_order(lhs->sym, rhs->sym);
}

static xkb_mod_mask_t xkb_mod_bit(xkb_mod_index_t mod)
{
	return (xkb_mod_mask_t) 1 << mod;
}

static unsigned int xkb_mask_weight(xkb_mod_mask_t mask)
{
	unsigned int weight = 0;

	while (mask) {
		weight += (unsigned int) (mask & 1u);
		mask >>= 1;
	}

	return weight;
}

static int parse_hexcode(struct lk_ctx *ctx, const char *symname)
{
	int value;
	size_t ret = strlen(symname);

	if (ret >= 4 && symname[0] == '0' && symname[1] == 'x') {
		errno = 0;
		value = (int) strtol(symname + 2, NULL, 16);
		if (errno == ERANGE) {
			XKEYMAP_WARNING(0, _("unable to convert unnamed non-Unicode xkb symbol `%s'"), symname);
			return -1;
		}
		return lk_convert_code(ctx, value, TO_UNICODE);
	}

	if (ret >= 5 && symname[0] == 'U' &&
	   isxdigit(symname[1]) && isxdigit(symname[2]) &&
	   isxdigit(symname[3]) && isxdigit(symname[4])) {
		errno = 0;
		value = (int) strtol(symname + 1, NULL, 16);
		if (errno == ERANGE) {
			XKEYMAP_WARNING(0, _("unable to convert unnamed unicode xkb symbol `%s'"), symname);
			return -1;
		}
		return lk_convert_code(ctx, value ^ 0xf000, TO_UNICODE);
	}

	return 0;
}

struct builtin_keysym_map {
	xkb_keysym_t sym;
	const char *kbd_name;
};

static int xkeymap_validate_code(int ret)
{
	if (ret <= 0 || ret > USHRT_MAX)
		return -1;

	return ret;
}

static int xkeymap_lookup_builtin_name(struct xkeymap *xkeymap, const char *name)
{
	if (!lk_valid_ksym(xkeymap->ctx, name, TO_8BIT))
		return -1;

	return lk_ksym_to_code(xkeymap->ctx, name, TO_8BIT);
}

static int xkeymap_get_code_from_unicode(struct xkeymap *xkeymap, xkb_keysym_t symbol)
{
	uint32_t xkb_unicode;

	(void) xkeymap;

	xkb_unicode = xkb_keysym_to_utf32(symbol);
	if (xkb_unicode < 0x20 || xkb_unicode == 0x7f)
		return -1;

	return (int) (xkb_unicode ^ 0xf000);
}

static int xkeymap_lookup_semantic_keysym(struct xkeymap *xkeymap, xkb_keysym_t symbol,
					  const struct builtin_keysym_map *map,
					  size_t map_size)
{
	for (size_t i = 0; i < map_size; i++) {
		if (map[i].sym == symbol)
			return xkeymap_lookup_builtin_name(xkeymap, map[i].kbd_name);
	}

	return -1;
}

static int xkeymap_get_code_from_semantic_keysym(struct xkeymap *xkeymap, xkb_keysym_t symbol)
{
	/* XKB dead keys need the distinct kernel actions, not legacy aliases. */
	static const struct builtin_keysym_map semantic_dead_map[] = {
		{ XKB_KEY_dead_caron,       "dead_kcaron"       },
		{ XKB_KEY_dead_breve,       "dead_kbreve"       },
		{ XKB_KEY_dead_doubleacute, "dead_kdoubleacute" },
		{ XKB_KEY_dead_ogonek,      "dead_kogonek"      },
	};
	static const struct builtin_keysym_map semantic_modifier_map[] = {
		{ XKB_KEY_Shift_L,		"Shift" },
		{ XKB_KEY_Shift_R,		"Shift" },
		{ XKB_KEY_Control_L,		"Control" },
		{ XKB_KEY_Control_R,		"Control" },
		{ XKB_KEY_Alt_L,		"Alt" },
		{ XKB_KEY_Alt_R,		"Alt" },
		{ XKB_KEY_Meta_L,		"Alt" },
		{ XKB_KEY_Meta_R,		"Alt" },
		{ XKB_KEY_Super_L,		"Alt" },
		{ XKB_KEY_Super_R,		"Alt" },
		{ XKB_KEY_Hyper_L,		"Alt" },
		{ XKB_KEY_Hyper_R,		"Alt" },
		{ XKB_KEY_Mode_switch,		"CtrlR" },
		{ XKB_KEY_Multi_key,		"Compose" },
		{ XKB_KEY_Sys_Req,		"Last_Console" },
		{ XKB_KEY_Print,		"Control_backslash" },
		{ XKB_KEY_Delete,		"Remove" },
		{ XKB_KEY_ISO_Level2_Latch,	"Shift" },
		{ XKB_KEY_ISO_Lock,		"Caps_Lock" },
		{ XKB_KEY_ISO_Level3_Shift,	"AltGr" },
		{ XKB_KEY_ISO_Level3_Latch,	"AltGr" },
		{ XKB_KEY_ISO_Level3_Lock,	"AltGr_Lock" },
		{ XKB_KEY_ISO_Level5_Shift,	"AltGr" },
		{ XKB_KEY_ISO_Level5_Latch,	"AltGr" },
		{ XKB_KEY_ISO_Level5_Lock,	"AltGr_Lock" },
		{ XKB_KEY_ISO_Group_Latch,	"ShiftL" },
		{ XKB_KEY_ISO_Group_Lock,	"ShiftL_Lock" },
		{ XKB_KEY_ISO_First_Group,	"ShiftL_Lock" },
		{ XKB_KEY_ISO_First_Group_Lock,	"ShiftL_Lock" },
		{ XKB_KEY_ISO_Last_Group,	"ShiftL_Lock" },
		{ XKB_KEY_ISO_Last_Group_Lock,	"ShiftL_Lock" },
		{ XKB_KEY_ISO_Next_Group,	"ShiftL_Lock" },
		{ XKB_KEY_ISO_Next_Group_Lock,	"ShiftL_Lock" },
		{ XKB_KEY_ISO_Prev_Group,	"ShiftL_Lock" },
		{ XKB_KEY_ISO_Prev_Group_Lock,	"ShiftL_Lock" },
	};
	static const struct builtin_keysym_map semantic_approximation_map[] = {
		{ XKB_KEY_ISO_Left_Tab,		"Meta_Tab" },
		{ XKB_KEY_Sys_Req,		"Last_Console" },
		{ XKB_KEY_Print,		"Control_backslash" },
		{ XKB_KEY_Delete,		"Remove" },
		{ XKB_KEY_KP_Insert,		"KP_0" },
		{ XKB_KEY_KP_End,		"KP_1" },
		{ XKB_KEY_KP_Down,		"KP_2" },
		{ XKB_KEY_KP_Next,		"KP_3" },
		{ XKB_KEY_KP_Left,		"KP_4" },
		{ XKB_KEY_KP_Begin,		"KP_5" },
		{ XKB_KEY_KP_Right,		"KP_6" },
		{ XKB_KEY_KP_Home,		"KP_7" },
		{ XKB_KEY_KP_Up,		"KP_8" },
		{ XKB_KEY_KP_Prior,		"KP_9" },
		{ XKB_KEY_KP_Delete,		"KP_Period" },
		{ XKB_KEY_KP_Decimal,		"KP_Comma" },
	};
	char console[16];
	int ret;

	ret = xkeymap_lookup_semantic_keysym(xkeymap, symbol, semantic_dead_map,
					     ARRAY_SIZE(semantic_dead_map));
	if (ret >= 0)
		return ret;

	/* Prefer direct semantic actions before Linux VT-specific approximations. */
	ret = xkeymap_lookup_semantic_keysym(xkeymap, symbol, semantic_modifier_map,
					     ARRAY_SIZE(semantic_modifier_map));
	if (ret >= 0)
		return ret;

	/*
	 * These remaps are Linux VT approximations rather than direct semantic
	 * equivalents: keypad navigation aliases collapse to keypad digits,
	 * Delete maps to the kernel Remove action, and VT switching uses the
	 * nearest console actions.
	 */
	ret = xkeymap_lookup_semantic_keysym(xkeymap, symbol, semantic_approximation_map,
					     ARRAY_SIZE(semantic_approximation_map));
	if (ret >= 0)
		return ret;

	if (symbol >= XKB_KEY_XF86Switch_VT_1 && symbol <= XKB_KEY_XF86Switch_VT_12) {
		if (snprintf(console, sizeof(console), "Console_%u",
			     (unsigned int) (symbol - XKB_KEY_XF86Switch_VT_1 + 1)) >= (int) sizeof(console))
			return -1;
		return xkeymap_lookup_builtin_name(xkeymap, console);
	}

	return -1;
}

static int xkeymap_get_code_from_name(struct xkeymap *xkeymap, xkb_keysym_t symbol)
{
	int ret;
	char symbuf[BUFSIZ];

	symbuf[0] = 0;

	ret = xkb_keysym_get_name(symbol, symbuf, sizeof(symbuf));

	if (ret < 0 || (size_t)ret >= sizeof(symbuf)) {
		XKEYMAP_WARNING(0, _("failed to get name of keysym"));
		return -1;
	}

	/*
	 * Resolve lexical aliases through libkeymap's normal symbol tables
	 * and synonyms. XKB-specific semantic remaps are handled separately.
	 */
	if (lk_valid_ksym(xkeymap->ctx, symbuf, TO_8BIT))
		ret = lk_ksym_to_code(xkeymap->ctx, symbuf, TO_8BIT);
	else
		ret = -1;

	/*
	 * The 8-bit name lookup may borrow a byte from another charset.
	 * Keep that representation only if it still denotes the named character
	 * in the current charset; otherwise retain the Unicode name resolution.
	 * Kernel actions must keep their symbol type.
	 */
	if (ret >= 0x80 && KTYP(ret) == KT_LATIN) {
		int unicode = lk_ksym_to_code(xkeymap->ctx, symbuf, TO_UNICODE);

		if (unicode >= 0x1000 &&
		    lk_convert_code(xkeymap->ctx, ret, TO_UNICODE) != (unicode ^ 0xf000))
			ret = unicode;
	}

	return xkeymap_validate_code(ret);
}

static int xkeymap_get_code(struct xkeymap *xkeymap, xkb_keysym_t symbol)
{
	int ret;
	char symbuf[BUFSIZ];

	/*
	 * The VT reserves U+F000..U+FFFF for actions and stores 16-bit values.
	 * Reject unrepresentable characters before name and numeric fallbacks
	 * can reinterpret their XOR encoding as a kernel action or an ASCII byte.
	 */
	if (xkb_keysym_to_utf32(symbol) >= 0xf000)
		return -1;

	ret = xkeymap_get_code_from_semantic_keysym(xkeymap, symbol);
	if (ret >= 0)
		return xkeymap_validate_code(ret);

	ret = xkeymap_get_code_from_name(xkeymap, symbol);
	if (ret >= 0)
		return xkeymap_validate_code(ret);

	ret = xkeymap_get_code_from_unicode(xkeymap, symbol);
	if (ret >= 0)
		return xkeymap_validate_code(ret);

	symbuf[0] = '\0';
	ret = xkb_keysym_get_name(symbol, symbuf, sizeof(symbuf));
	if (ret < 0 || (size_t) ret >= sizeof(symbuf)) {
		XKEYMAP_WARNING(0, _("failed to get name of keysym"));
		return -1;
	}

	return xkeymap_validate_code(parse_hexcode(xkeymap->ctx, symbuf));
}

static int xkeymap_get_symbol(struct xkb_keymap *keymap,
		xkb_keycode_t keycode, xkb_layout_index_t layout, xkb_level_index_t level,
		xkb_keysym_t *symbol)
{
	int num_syms;
	const xkb_keysym_t *syms = NULL;

	if (!(num_syms = xkb_keymap_key_get_syms_by_level(keymap, keycode, layout, level, &syms)))
		return 0;

	if (num_syms > 1) {
		XKEYMAP_WARNING(0, _("keycode %u layout %u level %u has %d symbols; using the first one"),
				keycode, layout, level, num_syms);
	}

	*symbol = syms[0];
	return 1;
}

static struct reachable_sym *lookup_reachable_sym(struct xkeymap *xkeymap, xkb_keysym_t sym)
{
	struct reachable_sym key = {
		.sym = sym,
	};
	struct reachable_sym **val;

	val = tfind(&key, &xkeymap->reachable_syms, compare_reachable_syms);
	if (!val)
		return NULL;

	return *val;
}

static void remember_reachable_sym(struct xkeymap *xkeymap, xkb_keysym_t sym, int code)
{
	struct reachable_sym *ptr, **val;

	if (lookup_reachable_sym(xkeymap, sym))
		return;

	ptr = malloc(sizeof(*ptr));
	if (!ptr)
		kbd_error(1, errno, _("out of memory"));

	ptr->sym = sym;
	ptr->code = code;

	val = tsearch(ptr, &xkeymap->reachable_syms, compare_reachable_syms);
	if (!val)
		kbd_error(1, errno, _("out of memory"));

	if (*val != ptr)
		free(ptr);
}

static int xkeymap_is_capslockable(xkb_keysym_t sym, int code)
{
	/* Only synthesize CapsLock behaviour for symbols the kernel can case-map. */
	if (xkb_keysym_to_lower(sym) == sym && xkb_keysym_to_upper(sym) == sym)
		return 0;

	if (code < 0x100)
		return 1;

	if (code >= 0x1000 && (code ^ 0xf000) < 0x100)
		return 1;

	return 0;
}

/* Convert modifiers left over after XKB has selected the key's level. */
static int xkeymap_apply_modifiers(struct xkeymap *xkeymap, struct xkb_state *state,
				   xkb_keycode_t key, unsigned int mods, int code)
{
	xkb_mod_mask_t consumed = xkb_state_key_get_consumed_mods(state, key);
	const xkb_keysym_t *syms;
	int count = xkb_state_key_get_syms(state, key, &syms);

	if ((mods & (1u << KG_CTRL)) &&
	    !(consumed & xkeymap->modifier_masks[XKEYMAP_MASK_CONTROL]) &&
	    count == 1 && xkb_keysym_to_utf32(syms[0]) &&
	    (code >= 0x1000 || KTYP(code) == KT_LATIN || KTYP(code) == KT_LETTER)) {
		uint32_t unicode = xkb_state_key_get_utf32(state, key);

		if (unicode < 0x80)
			code = K(KT_LATIN, (int) unicode);
	}

	/* Unconsumed Alt retains the console's Meta behavior. */
	if ((mods & (1u << KG_ALT)) &&
	    !(consumed & xkeymap->modifier_masks[XKEYMAP_MASK_ALT])) {
		if (code >= 0x1000 && (code ^ 0xf000) < 0x80)
			code = K(KT_META, code ^ 0xf000);
		else if ((KTYP(code) == KT_LATIN || KTYP(code) == KT_LETTER) &&
			 KVAL(code) < 0x80)
			code = K(KT_META, KVAL(code));
	}

	return code;
}

enum xkeymap_group_feature {
	XKEYMAP_GROUP_SWITCH = 1,
	XKEYMAP_GROUP_SELECT = 2,
};

static unsigned int xkeymap_group_features(struct xkb_keymap *keymap, xkb_keycode_t key)
{
	unsigned int features = 0;

	for (xkb_layout_index_t layout = 0; layout < xkb_keymap_num_layouts_for_key(keymap, key); layout++) {
		for (xkb_level_index_t level = 0; level < xkb_keymap_num_levels_for_key(keymap, key, layout); level++) {
			const xkb_keysym_t *syms;

			if (xkb_keymap_key_get_syms_by_level(keymap, key, layout, level, &syms) <= 0)
				continue;
			/* Mode_switch and ISO_Group_Shift are aliases. */
			if (syms[0] == XKB_KEY_Mode_switch)
				features |= XKEYMAP_GROUP_SWITCH;
			else if (syms[0] == XKB_KEY_ISO_First_Group || syms[0] == XKB_KEY_ISO_Last_Group ||
				 syms[0] == XKB_KEY_ISO_Prev_Group)
				features |= XKEYMAP_GROUP_SELECT;
		}
	}
	return features;
}

static int xkeymap_select_group(unsigned int num_layouts, unsigned int group, unsigned int target)
{
	static const int actions[] = { K_HOLE, K_SHIFTLLOCK, K_SHIFTRLOCK, K_CTRLLLOCK };

	/*
	 * A console lock action toggles only one bit. CtrlL inverts both bits
	 * in the decoded group index, allowing any target in one action.
	 * Selecting the current group must not toggle anything.
	 */
	for (unsigned int mask = 0; mask < ARRAY_SIZE(actions); mask++) {
		if (layouts[num_layouts - 1][group ^ mask] == target)
			return actions[mask];
	}
	return K_HOLE;
}

static int xkeymap_walk(struct xkeymap *xkeymap)
{
	struct xkb_state *state;
	lk_keywords keywords = lk_get_keywords(xkeymap->ctx);
	xkb_mod_mask_t masks[16] = { 0 };
	bool group_switch[NR_KEYS] = { false };
	bool has_group_switch = false;
	unsigned int group_features = 0;
	unsigned int group_states;
	xkb_layout_index_t num_layouts = xkb_keymap_num_layouts(xkeymap->keymap);
	xkb_keycode_t min = xkb_keymap_min_keycode(xkeymap->keymap);
	xkb_keycode_t max = xkb_keymap_max_keycode(xkeymap->keymap);
	int shift = lk_ksym_to_unicode(xkeymap->ctx, "Shift");
	int shiftl = lk_ksym_to_unicode(xkeymap->ctx, "ShiftL");
	int shiftr = lk_ksym_to_unicode(xkeymap->ctx, "ShiftR");
	int shift_lock = lk_ksym_to_unicode(xkeymap->ctx, "Shift_Lock");
	int shiftl_lock = lk_ksym_to_unicode(xkeymap->ctx, "ShiftL_Lock");
	int shiftr_lock = lk_ksym_to_unicode(xkeymap->ctx, "ShiftR_Lock");
	int ret = -1;

	if (num_layouts == 0 || num_layouts > NR_LAYOUTS) {
		XKEYMAP_WARNING(0, _("unable to convert XKB layouts: unsupported layout count %u"),
				(unsigned int) num_layouts);
		return -1;
	}

	state = xkb_state_new(xkeymap->keymap);
	if (!state)
		return -1;

	if (min < EVDEV_OFFSET)
		min = EVDEV_OFFSET;
	if (max >= NR_KEYS + EVDEV_OFFSET)
		max = NR_KEYS + EVDEV_OFFSET - 1;

	for (xkb_keycode_t key = min; key <= max; key++) {
		unsigned int features = xkeymap_group_features(xkeymap->keymap, key);

		group_switch[KERN_KEYCODE(key)] = (features & XKEYMAP_GROUP_SWITCH) != 0;
		group_features |= features;
	}

	has_group_switch = (group_features & XKEYMAP_GROUP_SWITCH) != 0;
	group_states = NR_LAYOUTS;
	if (num_layouts > 2 && (group_features & XKEYMAP_GROUP_SELECT))
		group_states *= 2;

	/* Shift, AltGr, Control and Alt occupy the four low console bits. */
	for (unsigned int mods = 0; mods < ARRAY_SIZE(masks); mods++) {
		for (size_t i = 0; i < ARRAY_SIZE(xkeymap_modifier_rules); i++) {
			const struct xkeymap_modifier_rule *rule = &xkeymap_modifier_rules[i];

			if (mods & rule->kernel_bit)
				masks[mods] |= xkeymap->modifier_masks[rule->mask];
		}
	}

	/* Every Meta binding is resolved below; do not synthesize one early. */
	lk_set_keywords(xkeymap->ctx, keywords & ~(unsigned int) LK_KEYWORD_ALTISMETA);

	for (xkb_keycode_t key = min; key <= max; key++) {
		/* Let XKB resolve per-key layout fallback for every console group. */
		for (unsigned int slot = 0; slot < group_states * (has_group_switch ? 2u : 1u); slot++) {
			unsigned int base = slot % group_states;
			unsigned int group = (base % NR_LAYOUTS) ^ (base >= NR_LAYOUTS ? 3u : 0u);
			bool held = slot >= group_states;
			xkb_layout_index_t selected = (layouts[num_layouts - 1][group] + held) % num_layouts;
			unsigned int group_table = layout_switch[base % NR_LAYOUTS] |
						   (base >= NR_LAYOUTS ? (1U << KG_CTRLL) : 0U);

			for (unsigned int mods = 0; mods < ARRAY_SIZE(masks); mods++) {
				xkb_layout_index_t layout;
				xkb_level_index_t level;
				xkb_keysym_t sym;
				unsigned int table = group_table | mods | (held ? (1U << KG_CTRLR) : 0U);
				int code;

				/*
				 * The console looks up releases in the current table.
				 * Keep the switch action while held, even if Shift now
				 * selects Compose or the new group has another binding.
				 * KG_CTRLR is separate from the locked group bits.
				 */
				if (held && group_switch[KERN_KEYCODE(key)]) {
					if (lk_add_key(xkeymap->ctx, table, KERN_KEYCODE(key), K_CTRLR) < 0)
						goto end;
					continue;
				}

				/* Level masks omit modifiers ignored by a key's type. */
				xkb_state_update_mask(state, masks[mods], 0, 0, 0, 0,
						      selected);
				layout = xkb_state_key_get_layout(state, key);
				if (layout == XKB_LAYOUT_INVALID)
					continue;
				level = xkb_state_key_get_level(state, key, layout);
				if (level == XKB_LEVEL_INVALID ||
				    xkeymap_get_symbol(xkeymap->keymap, key, layout, level, &sym) <= 0)
					continue;

				if (sym == XKB_KEY_ISO_Next_Group) {
					code = (group == 0 || group == 3) ? shiftl_lock : shiftr_lock;
				} else if (sym == XKB_KEY_ISO_Prev_Group) {
					/* LockGroup(-1) changes the locked group, not the held offset. */
					unsigned int target = (layouts[num_layouts - 1][group] + num_layouts - 1) % num_layouts;

					code = xkeymap_select_group(num_layouts, group, target);
				} else if (sym == XKB_KEY_ISO_First_Group || sym == XKB_KEY_ISO_Last_Group) {
					/* ISO_Last_Group conventionally locks group 2, not the final group. */
					unsigned int target = (sym == XKB_KEY_ISO_Last_Group) ? 1u % num_layouts : 0;

					code = xkeymap_select_group(num_layouts, group, target);
				} else {
					code = xkeymap_get_code(xkeymap, sym);
					if (code < 0)
						continue;

					/* Reserve ShiftL/ShiftR for layout switching. */
					if (code == shiftl || code == shiftr)
						code = shift;
					if (code == shiftl_lock || code == shiftr_lock)
						code = shift_lock;

					remember_reachable_sym(xkeymap, sym, code);
					if (xkeymap_is_capslockable(sym, code))
						code = lk_add_capslock(xkeymap->ctx, code);
					code = xkeymap_apply_modifiers(xkeymap, state, key, mods, code);
				}

				if (lk_add_key(xkeymap->ctx, table, KERN_KEYCODE(key), code) < 0)
					goto end;
			}
		}
	}
	ret = 0;
end:
	lk_set_keywords(xkeymap->ctx, keywords);
	xkb_state_unref(state);
	return ret;
}

static int xkeymap_fill_modifier_release_bindings(struct xkeymap *xkeymap)
{
	struct lk_ctx *ctx = xkeymap->ctx;

	/*
	 * This intentionally runs after xkeymap_walk(). Whether a pure
	 * modifier key must be visible in a given kernel table depends on the
	 * final set of tables that survived XKB conversion, and that set is
	 * only stable once the walk has finished.
	 */
	for (unsigned int keycode = 0; keycode < NR_KEYS; keycode++) {
		int modifier_code = K_HOLE;
		int seen = 0;

		for (unsigned int table = 0; table < MAX_NR_KEYMAPS; table++) {
			int code;

			if (!lk_map_exists(ctx, table) || !lk_key_exists(ctx, table, keycode))
				continue;

			code = lk_get_key(ctx, table, keycode);
			if (code == K_HOLE)
				continue;

			if (!seen) {
				modifier_code = code;
				seen = 1;
				continue;
			}

			if (code != modifier_code) {
				seen = 0;
				break;
			}
		}

		if (!seen || KTYP(modifier_code) != KT_SHIFT)
			continue;

		/*
		 * Linux resolves key release using the current modifier state,
		 * i.e. while the key being released is still considered active.
		 * For pure modifier keys, missing entries in shifted/control/alt
		 * tables therefore turn key release into VoidSymbol and leave
		 * the modifier stuck. Mirror the same modifier action into every
		 * already-allocated table unless the key already has a distinct
		 * binding there.
		 */
		for (unsigned int table = 0; table < MAX_NR_KEYMAPS; table++) {
			if (!lk_map_exists(ctx, table) || lk_key_exists(ctx, table, keycode))
				continue;

			if (lk_add_key(ctx, table, keycode, modifier_code) < 0)
				return -1;
		}
	}

	return 0;
}

static int xkeymap_compose_candidate_append(struct compose_candidate **candidates,
					    size_t *count, size_t *capacity,
					    const struct compose_candidate *candidate)
{
	struct compose_candidate *tmp;
	size_t new_capacity;

	if (*count == *capacity) {
		new_capacity = *capacity ? (*capacity * 2) : 32;
		tmp = realloc(*candidates, new_capacity * sizeof(**candidates));
		if (!tmp) {
			XKEYMAP_WARNING(errno, _("out of memory"));
			return -1;
		}
		*candidates = tmp;
		*capacity = new_capacity;
	}

	(*candidates)[(*count)++] = *candidate;
	return 0;
}

static int xkeymap_is_dead_keysym(xkb_keysym_t sym)
{
	char buf[128];
	int ret;

	ret = xkb_keysym_get_name(sym, buf, sizeof(buf));
	if (ret <= 0 || (size_t) ret >= sizeof(buf))
		return 0;

	return strncmp(buf, "dead_", 5) == 0;
}

static uint32_t xkeymap_compose_code_to_unicode(unsigned int code)
{
	if (code >= 0x1000)
		return code ^ 0xf000;

	if (KTYP(code) == KT_LATIN || KTYP(code) == KT_LETTER)
		return KVAL(code);

	return 0;
}

static uint32_t xkeymap_compose_result_unicode(const struct compose_candidate *candidate)
{
	/*
	 * xkeymap_get_code() already normalized the result into the kernel
	 * representation we are going to load, so score the candidate based on
	 * that value instead of trying to reinterpret the original XKB symbol.
	 */
	return xkeymap_compose_code_to_unicode(candidate->diacr.result);
}

static int xkeymap_is_ascii_lower(uint32_t c)
{
	return c >= 'a' && c <= 'z';
}

static int xkeymap_is_ascii_upper(uint32_t c)
{
	return c >= 'A' && c <= 'Z';
}

static int xkeymap_is_ascii_vowel(uint32_t c)
{
	switch (c) {
	case 'A': case 'E': case 'I': case 'O': case 'U': case 'Y':
	case 'a': case 'e': case 'i': case 'o': case 'u': case 'y':
		return 1;
	default:
		return 0;
	}
}

static int xkeymap_ascii_in_set(uint32_t c, const char *set)
{
	for (; *set; set++) {
		if (c == (unsigned char) *set)
			return 1;
	}

	return 0;
}

static int xkeymap_is_preferred_console_dead_rule(const struct compose_candidate *candidate)
{
	uint32_t base_unicode = xkeymap_compose_code_to_unicode(candidate->diacr.base);

	/* A dead key encoded as an ASCII accent is not a base letter. */
	if (xkeymap_is_dead_keysym(candidate->seq[1]))
		return 0;

	/*
	 * Prefer the compact Latin dead-key repertoire used by existing
	 * console keymaps and console-setup dkey tables.  This keeps the
	 * kernel compose table biased toward historically expected accent
	 * combinations instead of locale-wide XKB noise.
	 */
	switch (candidate->seq[0]) {
	case XKB_KEY_dead_grave:
	case XKB_KEY_dead_macron:
		return xkeymap_ascii_in_set(base_unicode, "AEIOUaeiou");
	case XKB_KEY_dead_acute:
		return xkeymap_ascii_in_set(base_unicode, "ACEILNORSUYZaceilnorsuyz");
	case XKB_KEY_dead_circumflex:
		return xkeymap_ascii_in_set(base_unicode, "ACEGHIJOSUWYaceghijosuwy");
	case XKB_KEY_dead_tilde:
		return xkeymap_ascii_in_set(base_unicode, "AINOUainou");
	case XKB_KEY_dead_diaeresis:
		return xkeymap_is_ascii_vowel(base_unicode);
	case XKB_KEY_dead_abovering:
		return xkeymap_ascii_in_set(base_unicode, "AUau");
	case XKB_KEY_dead_cedilla:
		return xkeymap_ascii_in_set(base_unicode, "ACESTacest");
	case XKB_KEY_dead_caron:
		return xkeymap_ascii_in_set(base_unicode, "CDELNRSTZcdelnrstz");
	case XKB_KEY_dead_ogonek:
		return xkeymap_ascii_in_set(base_unicode, "AEae");
	case XKB_KEY_dead_breve:
		return xkeymap_ascii_in_set(base_unicode, "AGUagu");
	case XKB_KEY_dead_doubleacute:
		return xkeymap_ascii_in_set(base_unicode, "OUou");
	case XKB_KEY_dead_abovedot:
		return xkeymap_ascii_in_set(base_unicode, "CEGILZcegilz");
	default:
		return 0;
	}
}

static int xkeymap_score_compose_candidate(const struct compose_candidate *candidate)
{
	uint32_t base_unicode = xkeymap_compose_code_to_unicode(candidate->diacr.base);
	uint32_t result_unicode = xkeymap_compose_result_unicode(candidate);
	int score = 0;

	if (xkeymap_is_dead_keysym(candidate->seq[1]))
		base_unicode = 0;

	/*
	 * Prefer classic dead-key style sequences first: these are the rules
	 * users are most likely to expect from a compact kernel compose table.
	 */
	if (xkeymap_is_dead_keysym(candidate->seq[0]))
		score += 1000;

	if (xkeymap_is_preferred_console_dead_rule(candidate))
		score += 300;

	/*
	 * Under MAX_DIACR, accent + letter rules are more useful than accent +
	 * digit/space/keypad rules and closer to traditional console keymaps.
	 */
	if (xkeymap_is_ascii_lower(base_unicode))
		score += 250;
	else if (xkeymap_is_ascii_upper(base_unicode))
		score += 200;
	else if ((base_unicode >= '0' && base_unicode <= '9') || base_unicode == ' ')
		score -= 200;
	else if (base_unicode != 0)
		score -= 50;

	/* Latin-1 and Latin Extended-A are the most valuable within MAX_DIACR. */
	if (result_unicode > 0 && result_unicode <= 0x00ff)
		score += 200;
	else if (result_unicode > 0 && result_unicode <= 0x017f)
		score += 100;
	else if (result_unicode > 0)
		score += 25;

	/*
	 * A directly reachable symbol is less valuable than one compose adds
	 * from scratch, but traditional console keymaps still keep important
	 * dead-key combinations for directly typed national letters.
	 */
	if (candidate->result_reachable)
		score -= 100;

	return score;
}

/* Scores can be negative; keysym and character ordering remains unsigned. */
static int compare_compose_scores(int lhs, int rhs)
{
	if (lhs < rhs)
		return -1;
	if (lhs > rhs)
		return 1;
	return 0;
}

static int compare_compose_candidates(const void *pa, const void *pb)
{
	const struct compose_candidate *lhs = pa;
	const struct compose_candidate *rhs = pb;
	int ret;

	ret = compare_compose_scores(rhs->score, lhs->score);
	if (ret != 0)
		return ret;

	ret = compare_compose_order(lhs->seq[0], rhs->seq[0]);
	if (ret != 0)
		return ret;

	ret = compare_compose_order(lhs->seq[1], rhs->seq[1]);
	if (ret != 0)
		return ret;

	return compare_compose_order(lhs->result_sym, rhs->result_sym);
}

static int compare_compose_candidates_by_sequence(const void *pa, const void *pb)
{
	const struct compose_candidate *lhs = pa;
	const struct compose_candidate *rhs = pb;
	int ret;

	ret = compare_compose_order(lhs->seq[0], rhs->seq[0]);
	if (ret != 0)
		return ret;

	ret = compare_compose_order(lhs->seq[1], rhs->seq[1]);
	if (ret != 0)
		return ret;

	ret = compare_compose_scores(rhs->score, lhs->score);
	if (ret != 0)
		return ret;

	return compare_compose_order(lhs->result_sym, rhs->result_sym);
}

static int compose_candidates_same_sequence(const struct compose_candidate *lhs,
					    const struct compose_candidate *rhs)
{
	return lhs->seq[0] == rhs->seq[0] &&
	       lhs->seq[1] == rhs->seq[1];
}

static int compare_kernel_compose_rules(const void *pa, const void *pb)
{
	const struct lk_kbdiacr *lhs = pa;
	const struct lk_kbdiacr *rhs = pb;
	int ret;

	ret = compare_compose_order(lhs->diacr, rhs->diacr);
	if (ret != 0)
		return ret;

	/* The kernel looks up the input pair and uses the first result. */
	return compare_compose_order(lhs->base, rhs->base);
}

static size_t xkeymap_select_compose_candidates(struct compose_candidate *candidates,
						size_t count)
{
	size_t out = 0;

	if (count < 2)
		return count;

	qsort(candidates, count, sizeof(*candidates), compare_compose_candidates_by_sequence);

	for (size_t i = 0; i < count; i++) {
		if (out > 0 && compose_candidates_same_sequence(&candidates[out - 1], &candidates[i]))
			continue;

		candidates[out++] = candidates[i];
	}

	qsort(candidates, out, sizeof(*candidates), compare_compose_candidates);

	return out;
}

static int xkeymap_symbol_is_reachable(struct xkeymap *xkeymap, xkb_keysym_t sym, int *code)
{
	struct reachable_sym *reachable;

	reachable = lookup_reachable_sym(xkeymap, sym);
	if (!reachable)
		return 0;

	if (code)
		*code = reachable->code;

	return 1;
}

static int xkeymap_compose_input_code(int code)
{
	/* Match the characters passed to k_deadunicode() by Linux k_dead(). */
	static const unsigned char dead_chars[] = {
		'`',  /* dead_grave */
		'\'', /* dead_acute */
		'^',  /* dead_circumflex */
		'~',  /* dead_tilde */
		'"',  /* dead_diaeresis */
		',',  /* dead_cedilla */
		'_',  /* dead_macron */
		'U',  /* dead_breve */
		'.',  /* dead_abovedot */
		'*',  /* dead_abovering */
		'=',  /* dead_doubleacute */
		'c',  /* dead_caron */
		'k',  /* dead_ogonek */
		'i',  /* dead_iota */
		'#',  /* dead_voiced_sound */
		'o',  /* dead_semivoiced_sound */
		'!',  /* dead_belowdot */
		'?',  /* dead_hook */
		'+',  /* dead_horn */
		'-',  /* dead_stroke */
		')',  /* dead_abovecomma */
		'(',  /* dead_abovereversedcomma */
		':',  /* dead_doublegrave */
		'n',  /* dead_invertedbreve */
		';',  /* dead_belowcomma */
		'$',  /* dead_currency */
		'@',  /* dead_greek */
	};

	if (KTYP(code) == KT_DEAD) {
		if (KVAL(code) >= ARRAY_SIZE(dead_chars))
			return -1;
		return dead_chars[KVAL(code)];
	}
	if (KTYP(code) == KT_DEAD2)
		return KVAL(code) ^ 0xf000;

	return code;
}

static int xkeymap_compose_candidate_from_entry(struct xkeymap *xkeymap,
						struct xkb_compose_table_entry *entry,
						struct compose_candidate *candidate)
{
	size_t seqlen = 0;
	const xkb_keysym_t *syms = xkb_compose_table_entry_sequence(entry, &seqlen);
	xkb_keysym_t keysym = xkb_compose_table_entry_keysym(entry);
	int code;

	if (keysym == XKB_KEY_NoSymbol || seqlen != 2)
		return 0;

	if (!xkeymap_symbol_is_reachable(xkeymap, syms[0], &code))
		return 0;
	code = xkeymap_compose_input_code(code);
	if (code < 0)
		return 0;
	candidate->diacr.diacr = (unsigned int) code;
	candidate->seq[0] = syms[0];

	if (!xkeymap_symbol_is_reachable(xkeymap, syms[1], &code))
		return 0;
	code = xkeymap_compose_input_code(code);
	if (code < 0)
		return 0;
	candidate->diacr.base = (unsigned int) code;
	candidate->seq[1] = syms[1];

	if (xkeymap_symbol_is_reachable(xkeymap, keysym, &code))
		candidate->result_reachable = 1;
	else {
		code = xkeymap_get_code(xkeymap, keysym);
		if (code < 0)
			return 0;
		candidate->result_reachable = 0;
	}

	candidate->diacr.result = (unsigned int) code;
	candidate->result_sym = keysym;
	candidate->score = xkeymap_score_compose_candidate(candidate);

	return 1;
}

static int xkeymap_collect_compose_candidates(struct xkeymap *xkeymap,
					      struct compose_candidate **candidates_out,
					      size_t *count_out)
{
	struct xkb_compose_table_entry *entry;
	struct xkb_compose_table_iterator *iter = xkb_compose_table_iterator_new(xkeymap->compose);
	struct compose_candidate *candidates = NULL;
	size_t count = 0, capacity = 0;
	int ret = 0;

	if (!iter) {
		XKEYMAP_WARNING(0, "xkb_compose_table_iterator_new failed");
		return -1;
	}

	while ((entry = xkb_compose_table_iterator_next(iter))) {
		struct compose_candidate candidate;

		ret = xkeymap_compose_candidate_from_entry(xkeymap, entry, &candidate);
		if (ret < 0)
			break;
		if (ret == 0)
			continue;

		if (xkeymap_compose_candidate_append(&candidates, &count, &capacity, &candidate) < 0) {
			ret = -1;
			break;
		}
	}

	xkb_compose_table_iterator_free(iter);

	if (ret < 0)
		goto err;

	*candidates_out = candidates;
	*count_out = count;
	return 0;
err:
	free(candidates);
	return -1;
}

static int xkeymap_append_compose_candidates(struct xkeymap *xkeymap,
					     const struct compose_candidate *candidates,
					     size_t count,
					     size_t *total_rules_out)
{
	struct lk_kbdiacr *rule, **val;
	void *seen_rules = NULL;
	size_t appended = 0, total_rules = 0;
	int ret = 0;
	int direction = TO_8BIT;

#ifdef KDSKBDIACRUC
	if (lk_get_parser_flags(xkeymap->ctx) & LK_FLAG_PREFER_UNICODE)
		direction = TO_UNICODE;
#endif

	for (size_t i = 0; i < count; i++) {
		struct lk_kbdiacr diacr;

		rule = malloc(sizeof(*rule));
		if (!rule) {
			ret = -1;
			break;
		}

		*rule = candidates[i].diacr;
		/* Compare inputs in the same encoding lk_append_compose() stores. */
		rule->diacr = (unsigned int) lk_convert_code(xkeymap->ctx, (int) rule->diacr, direction);
		rule->base = (unsigned int) lk_convert_code(xkeymap->ctx, (int) rule->base, direction);
		val = tsearch(rule, &seen_rules, compare_kernel_compose_rules);
		if (!val) {
			free(rule);
			ret = -1;
			break;
		}

		if (*val != rule) {
			free(rule);
			continue;
		}

		total_rules++;
		if (appended >= MAX_DIACR)
			continue;

		diacr = candidates[i].diacr;

		ret = lk_append_compose(xkeymap->ctx, &diacr);
		if (ret == -1)
			break;

		appended++;
	}

	if (total_rules_out)
		*total_rules_out = total_rules;

	if (seen_rules)
		tdestroy(seen_rules, free);

	if (ret == -1)
		return -1;

	return 0;
}

static int xkeymap_compose(struct xkeymap *xkeymap)
{
	struct compose_candidate *candidates = NULL;
	size_t count = 0, selected, kernel_rules = 0;
	int ret;

	ret = xkeymap_collect_compose_candidates(xkeymap, &candidates, &count);
	if (ret < 0)
		return -1;

	selected = xkeymap_select_compose_candidates(candidates, count);

	ret = xkeymap_append_compose_candidates(xkeymap, candidates, selected, &kernel_rules);
	if (ret == 0 && kernel_rules > MAX_DIACR) {
		XKEYMAP_WARNING(0, _("kernel can handle only %d compose definitions, but xkb needs %zu"),
				MAX_DIACR, kernel_rules);
	}
	free(candidates);

	return ret;
}

#define XKEYMAP_MAX_RMLVO_ITEMS 32

enum xkeymap_rmlvo_field {
	XKEYMAP_RMLVO_OK = 0,
	XKEYMAP_RMLVO_MODEL,
	XKEYMAP_RMLVO_LAYOUT,
	XKEYMAP_RMLVO_VARIANT,
};

/*
 * Does the registry list a layout with the given name, optionally restricted
 * to a specific variant? An empty variant matches the base layout (variant NULL).
 */
static bool
xkeymap_layout_is_known(struct rxkb_context *rxkb, const char *layout, const char *variant)
{
	struct rxkb_layout *l;

	if (!layout || !*layout)
		return true;

	for (l = rxkb_layout_first(rxkb); l; l = rxkb_layout_next(l)) {
		const char *v;

		if (strcmp(rxkb_layout_get_name(l), layout) != 0)
			continue;

		v = rxkb_layout_get_variant(l);
		if (variant == NULL || *variant == '\0') {
			if (v == NULL)
				return true;
		} else {
			if (v != NULL && strcmp(v, variant) == 0)
				return true;
		}
	}

	return false;
}

/*
 * Split `list` on commas into at most `cap` items.  Empty entries are
 * preserved, so ",foo" yields two items ("" and "foo"): they are valid in an
 * RMLVO list where the i-th variant applies to the i-th layout and a variant
 * may be omitted for any one layout.
 */
static int
xkeymap_tokenize(const char *list, char items[][64], size_t cap)
{
	const char *p = list, *start = list;
	size_t len;
	int count = 0;

	if (list == NULL || *list == '\0')
		return 0;

	for (p = list; ; p++) {
		if (*p == '\0' || *p == ',') {
			len = (size_t) (p - start);
			if (len >= sizeof(items[0]))
				len = sizeof(items[0]) - 1;
			memcpy(items[count], start, len);
			items[count][len] = '\0';
			count++;
			if (count >= (int) cap)
				break;
		}

		if (*p == '\0')
			break;
		if (*p == ',')
			start = p + 1;
	}

	return count;
}

/*
 * Validate the model/layout/variant RMLVO triple against the registry for
 * names->rules.  Layouts and variants are comma lists that are matched
 * positionally.  Returns XKEYMAP_RMLVO_OK on success, otherwise the first
 * unrecognized field.
 */
static enum xkeymap_rmlvo_field
xkeymap_rmlvo_check(const struct xkb_rule_names *names)
{
	char layout_items[XKEYMAP_MAX_RMLVO_ITEMS][64];
	char variant_items[XKEYMAP_MAX_RMLVO_ITEMS][64];
	int n_layout = 0, n_variant = 0, i;
	struct rxkb_context *rxkb;

	if (names->model == NULL || *names->model == '\0')
		return XKEYMAP_RMLVO_OK;

	rxkb = rxkb_context_new(RXKB_CONTEXT_LOAD_EXOTIC_RULES);
	if (rxkb == NULL)
		return XKEYMAP_RMLVO_MODEL;

	if (!rxkb_context_parse(rxkb, names->rules)) {
		rxkb_context_unref(rxkb);
		return XKEYMAP_RMLVO_MODEL;
	}

	struct rxkb_model *m;
	bool found = false;

	for (m = rxkb_model_first(rxkb); m; m = rxkb_model_next(m)) {
		if (strcmp(rxkb_model_get_name(m), names->model) == 0) {
			found = true;
			break;
		}
	}

	if (!found) {
		rxkb_context_unref(rxkb);
		return XKEYMAP_RMLVO_MODEL;
	}

	n_layout = xkeymap_tokenize(names->layout, layout_items, ARRAY_SIZE(layout_items));
	n_variant = xkeymap_tokenize(names->variant, variant_items, ARRAY_SIZE(variant_items));

	for (i = 0; i < n_layout; i++) {
		const char *v = (i < n_variant) ? variant_items[i] : "";

		if (!xkeymap_layout_is_known(rxkb, layout_items[i], v)) {
			rxkb_context_unref(rxkb);
			return XKEYMAP_RMLVO_LAYOUT;
		}
	}

	rxkb_context_unref(rxkb);
	return XKEYMAP_RMLVO_OK;
}

static bool xkeymap_has_shifts_toggle(const char *options)
{
	const char unsupported[] = "grp:shifts_toggle";

	/* A NULL option list lets libxkbcommon use the environment default. */
	if (!options)
		options = getenv("XKB_DEFAULT_OPTIONS");
	while (options && *options) {
		const char *next = strchr(options, ',');
		size_t len = strcspn(options, ",!");

		/* Ignore surrounding whitespace and an optional XKB group suffix. */
		while (len && isspace((unsigned char) *options)) {
			options++;
			len--;
		}
		while (len && isspace((unsigned char) options[len - 1]))
			len--;
		if (len == sizeof(unsupported) - 1 && memcmp(options, unsupported, len) == 0)
			return true;
		options = next ? next + 1 : NULL;
	}
	return false;
}

int convert_xkb_keymap(struct lk_ctx *ctx, struct xkeymap_params *params)
{
	struct xkeymap xkeymap = { 0 };
	int ret = -1;

	enum xkeymap_rmlvo_field rmlvo;

	struct xkb_rule_names names = {
		.rules = "evdev",
		.model = params->model,
		.layout = params->layout,
		.variant = params->variant,
		.options = params->options,
	};

	xkeymap.ctx = ctx;

	/* VT releases use the current table, so a group action can hide Shift release. */
	if (xkeymap_has_shifts_toggle(names.options)) {
		XKEYMAP_WARNING(0, _("unsupported xkb option `grp:shifts_toggle': switching groups with both Shift keys can leave Shift stuck on the Linux console"));
		goto end;
	}

	lk_set_keywords(ctx, LK_KEYWORD_ALTISMETA | LK_KEYWORD_STRASUSUAL);

	/*
	 * Sanity-check the RMLVO triple before compiling the keymap.  Bogus
	 * values used to slip through the evdev rules silently because every
	 * model maps to the same keycodes component and layout/variant are
	 * resolved through wildcards, so only the layout/variant compile step
	 * rejected them.  The registry gives us a precise, up-front check.
	 */
	rmlvo = xkeymap_rmlvo_check(&names);
	if (rmlvo == XKEYMAP_RMLVO_MODEL) {
		XKEYMAP_WARNING(0, _("unrecognized xkb model `%s'"),
				params->model);
		goto end;
	}
	if (rmlvo == XKEYMAP_RMLVO_LAYOUT) {
		XKEYMAP_WARNING(0, _("unrecognized xkb layout `%s'"),
				params->layout ? params->layout : "");
		goto end;
	}

	xkeymap.xkb = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	if (!xkeymap.xkb) {
		XKEYMAP_WARNING(0, "xkb_context_new failed");
		goto end;
	}

	xkeymap.keymap = xkb_keymap_new_from_names(xkeymap.xkb, &names, XKB_KEYMAP_COMPILE_NO_FLAGS);
	if (!xkeymap.keymap) {
		XKEYMAP_WARNING(0, "xkb_keymap_new_from_names failed");
		goto end;
	}

	if (xkb_keymap_num_layouts(xkeymap.keymap) > NR_LAYOUTS) {
		XKEYMAP_WARNING(0, _("too many layouts specified. At the moment, you can use no more than %d"), NR_LAYOUTS);
		goto end;
	}

	xkeymap_init_modifier_masks(&xkeymap);

	if (params->locale) {
		xkeymap.compose = xkb_compose_table_new_from_locale(xkeymap.xkb, params->locale, XKB_COMPOSE_COMPILE_NO_FLAGS);
		if (!xkeymap.compose) {
			XKEYMAP_WARNING(0, _("xkb compose table for locale `%s' was not found; continuing without compose support"),
					params->locale);
		}
	}

	if (xkeymap_walk(&xkeymap) < 0)
		goto end;

	if (xkeymap.compose && xkeymap_compose(&xkeymap) < 0)
		goto end;

	if (xkeymap_fill_modifier_release_bindings(&xkeymap) < 0)
		goto end;

	ret = 0;
end:
	if (xkeymap.reachable_syms)
		tdestroy(xkeymap.reachable_syms, free);

	if (xkeymap.compose)
		xkb_compose_table_unref(xkeymap.compose);

	if (xkeymap.keymap)
		xkb_keymap_unref(xkeymap.keymap);
	if (xkeymap.xkb)
		xkb_context_unref(xkeymap.xkb);

	return ret;
}
