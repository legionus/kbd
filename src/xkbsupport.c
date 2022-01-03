#include <stdlib.h>
#include <string.h>

#include "libcommon.h"
#include "keymap.h"
#include "loadkeys.h"
#include "xkbsupport.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

static const int layout_switch[] = {
	0,
	(1 << KG_SHIFTL),
	(1 << KG_SHIFTR),
	(1 << KG_SHIFTL) | (1 << KG_SHIFTR)
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
static const unsigned int layouts[4][4] = {
	{ 0, 0, 0, 0 },
	{ 0, 1, 1, 0 },
	{ 0, 1, 2, 0 },
	{ 0, 1, 3, 2 },
};

struct xkeymap {
	struct xkb_context *xkb;
	struct xkb_keymap *keymap;
	struct lk_ctx *ctx;
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

/*
 * Copied from libxkbcommon.
 * Don't allow more modifiers than we can hold in xkb_mod_mask_t.
 */
#define XKB_MAX_MODS ((xkb_mod_index_t)(sizeof(xkb_mod_mask_t) * 8))

struct xkb_mask {
	xkb_mod_mask_t mask[XKB_MAX_MODS];
	size_t num;
};

struct modifier_mapping {
	const char *xkb_mod;
	const char *krn_mod;
	const int bit;
};

static struct modifier_mapping modifier_mapping[] = {
	{ "Shift",    "shift",   (1u << KG_SHIFT) },
	{ "Control",  "control", (1u << KG_CTRL)  },
	{ "Lock",     "???",     0                },
	{ "Mod1",     "alt",     (1u << KG_ALT)   },
	{ "Mod2",     "???",     0                },
	{ "Mod3",     "???",     0                },
	{ "Mod4",     "???",     0                },
	{ "Mod5",     "altgr",   (1u << KG_ALT)   },
	{ NULL, NULL, 0 },
};

static struct modifier_mapping *convert_modifier(const char *xkb_name)
{
	struct modifier_mapping *map = modifier_mapping;

	while (map->xkb_mod) {
		if (!strcasecmp(map->xkb_mod, xkb_name))
			return map;
		map++;
	}

	return NULL;
}

struct symbols_mapping {
	const char *xkb_sym;
	const char *krn_sym;
};

static struct symbols_mapping symbols_mapping[] = {
	{ "0", "zero" },
	{ "1", "one" },
	{ "2", "two" },
	{ "3", "three" },
	{ "4", "four" },
	{ "5", "five" },
	{ "6", "six" },
	{ "7", "seven" },
	{ "8", "eight" },
	{ "9", "nine" },
	{ "KP_Insert", "KP_0" },
	{ "KP_End", "KP_1" },
	{ "KP_Down", "KP_2" },
	{ "KP_Next", "KP_3" },
	{ "KP_Left", "KP_4" },
	{ "KP_Right", "KP_6" },
	{ "KP_Home", "KP_7" },
	{ "KP_Up", "KP_8" },
	{ "KP_Prior", "KP_9" },
	{ "KP_Begin", "VoidSymbol" },
	{ "KP_Delete", "VoidSymbol" },
	{ "Alt_R", "Alt" },
	{ "Alt_L", "Alt" },
	{ "Control_R", "Control" },
	{ "Control_L", "Control" },
	{ "Super_R", "Alt" },
	{ "Super_L", "Alt" },
	{ "Hyper_R", "Alt" },
	{ "Hyper_L", "Alt" },
	{ "Mode_switch", "AltGr" },
	{ "ISO_Group_Shift", "AltGr" },
	{ "ISO_Group_Latch", "AltGr" },
	{ "ISO_Group_Lock", "AltGr_Lock" },
	{ "ISO_Next_Group", "AltGr_Lock" },
	{ "ISO_Next_Group_Lock", "AltGr_Lock" },
	{ "ISO_Prev_Group", "AltGr_Lock" },
	{ "ISO_Prev_Group_Lock", "AltGr_Lock" },
	{ "ISO_First_Group", "AltGr_Lock" },
	{ "ISO_First_Group_Lock", "AltGr_Lock" },
	{ "ISO_Last_Group", "AltGr_Lock" },
	{ "ISO_Last_Group_Lock", "AltGr_Lock" },
	{ "ISO_Level3_Shift", "AltGr" },
	{ "ISO_Left_Tab", "Meta_Tab" },
	{ "XF86Switch_VT_1", "Console_1" },
	{ "XF86Switch_VT_2", "Console_2" },
	{ "XF86Switch_VT_3", "Console_3" },
	{ "XF86Switch_VT_4", "Console_4" },
	{ "XF86Switch_VT_5", "Console_5" },
	{ "XF86Switch_VT_6", "Console_6" },
	{ "XF86Switch_VT_7", "Console_7" },
	{ "XF86Switch_VT_8", "Console_8" },
	{ "XF86Switch_VT_9", "Console_9" },
	{ "XF86Switch_VT_10", "Console_10" },
	{ "XF86Switch_VT_11", "Console_11" },
	{ "XF86Switch_VT_12", "Console_12" },
	{ "Sys_Req", "Last_Console" },
	{ "Print", "Control_backslash" },
	{ NULL, NULL },
};

static const char *convert_xkb_symbol(const char *xkb_sym)
{
	struct symbols_mapping *map = symbols_mapping;

	while (map->xkb_sym) {
		if (!strcmp(map->xkb_sym, xkb_sym))
			return map->krn_sym;
		map++;
	}

	return NULL;
}

static void print_modifiers(struct xkb_keymap *keymap, struct xkb_mask *mask)
{
	xkb_mod_index_t num_mods = xkb_keymap_num_mods(keymap);

	for (size_t m = 0; m < mask->num; m++) {
		for (xkb_mod_index_t mod = 0; mod < num_mods; mod++) {
			if ((mask->mask[m] & (1u << mod)) == 0)
				continue;

			const char *modname = xkb_keymap_mod_get_name(keymap, mod);

			printf("%ld:%s(%d) ", m, modname, mod);
		}
	}
}

static void xkeymap_keycode_mask(struct xkb_keymap *keymap,
                                 xkb_layout_index_t layout, xkb_level_index_t level, xkb_keycode_t keycode,
                                 struct xkb_mask *out)
{
	memset(out->mask, 0, sizeof(out->mask));
	out->num = xkb_keymap_key_get_mods_for_level(keymap, keycode, layout, level, out->mask, ARRAY_SIZE(out->mask));
}

static void xkeymap_walk_printer(struct xkeymap *xkeymap,
                                 xkb_layout_index_t layout, xkb_level_index_t level,
				 xkb_keycode_t keycode, xkb_keysym_t sym)
{
	switch (sym) {
		case XKB_KEY_ISO_Next_Group:
			printf("* ");
			break;
		default:
			printf("  ");
			break;
	}

	printf("{%-12s} %d ", xkb_keymap_layout_get_name(xkeymap->keymap, layout), layout);
	printf("%d ", level);

	char s[255];
	const char *symname = NULL;
	int ret, kunicode;
	uint32_t unicode = xkb_keysym_to_utf32(sym);

	ret = xkb_keysym_get_name(sym, s, sizeof(s));

	if (ret < 0 || (size_t)ret >= sizeof(s)) {
		kbd_warning(0, "failed to get name of keysym");
		return;
	}

	if (!(symname = convert_xkb_symbol(s)))
		symname = s;

	if (unicode == 0) {
		if ((kunicode = lk_ksym_to_unicode(xkeymap->ctx, symname)) < 0) {
			printf("keycode %3d = %s", KERN_KEYCODE(keycode), symname);
		} else {
			unicode = (uint32_t) kunicode;
		}
	}

	if (unicode > 0)
		printf("keycode %3d = U+%04x %-32s", KERN_KEYCODE(keycode), unicode, symname);

	printf(" ( ");

	struct xkb_mask keycode_mask;
	xkeymap_keycode_mask(xkeymap->keymap, layout, level, keycode, &keycode_mask);

	if (keycode_mask.num > 0) {
		xkb_mod_index_t num_mods = xkb_keymap_num_mods(xkeymap->keymap);

		for (xkb_mod_index_t mod = 0; mod < num_mods; mod++) {
			if ((keycode_mask.mask[0] & (1u << mod)) == 0)
				continue;

			const struct modifier_mapping *map = convert_modifier(xkb_keymap_mod_get_name(xkeymap->keymap, mod));

			printf("%s ", map->krn_mod);
		}
	}

	printf(") ( ");
	print_modifiers(xkeymap->keymap, &keycode_mask);
	printf(")\n");
}

static int xkeymap_get_code(struct lk_ctx *ctx, xkb_keysym_t symbol)
{
	uint32_t xkb_unicode;
	int ret, unicode, code;
	const char *symname;
	char symbuf[BUFSIZ];

	symbuf[0] = 0;

	ret = xkb_keysym_get_name(symbol, symbuf, sizeof(symbuf));

	if (ret < 0 || (size_t)ret >= sizeof(symbuf)) {
		kbd_warning(0, "failed to get name of keysym");
		return -1;
	}

	if ((symname = convert_xkb_symbol(symbuf)) != NULL) {
		if ((unicode = lk_ksym_to_unicode(ctx, symname)) < 0)
			return -1;
	} else if (!(xkb_unicode = xkb_keysym_to_utf32(symbol))) {
		if ((unicode = lk_ksym_to_unicode(ctx, symbuf)) < 0)
			return -1;
	} else {
		unicode = ((int) xkb_unicode) ^ 0xf000;
	}

	code = lk_convert_code(ctx, unicode, TO_AUTO);
	if (code < 0) {
		kbd_warning(0, "unable to convert code U+%04x", unicode);
		return -1;
	}

	return code;
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
		kbd_warning(0, "fixme: more %d symbol on level", num_syms);
		return -1;
	}

	*symbol = syms[0];
	return 1;
}

static void xkeymap_add_value(struct xkeymap *xkeymap, int modifier, int code, int keyvalue[MAX_NR_KEYMAPS])
{
	if (!modifier || (modifier & (1 << KG_SHIFT)))
		code = lk_add_capslock(xkeymap->ctx, code);

	keyvalue[modifier] = code;
}

static int xkeymap_walk(struct xkeymap *xkeymap)
{
	struct xkb_mask keycode_mask;
	xkb_mod_index_t num_mods = xkb_keymap_num_mods(xkeymap->keymap);
	xkb_keycode_t min_keycode = xkb_keymap_min_keycode(xkeymap->keymap);
	xkb_keycode_t max_keycode = xkb_keymap_max_keycode(xkeymap->keymap);

	if (KERN_KEYCODE(min_keycode) >= NR_KEYS) {
		kbd_warning(0, "keymap defines more keycodes than the kernel can handle.");
		min_keycode = (NR_KEYS - 1) + EVDEV_OFFSET;
	}

	if (KERN_KEYCODE(max_keycode) >= NR_KEYS) {
		kbd_warning(0, "keymap defines more keycodes than the kernel can handle.");
		max_keycode = (NR_KEYS - 1) + EVDEV_OFFSET;
	}

	xkb_layout_index_t num_layouts = xkb_keymap_num_layouts(xkeymap->keymap);

	for (xkb_keycode_t keycode = min_keycode; keycode <= max_keycode; keycode++) {
		int keyvalue[MAX_NR_KEYMAPS];

		memset(keyvalue, 0, sizeof(keyvalue));

		for (xkb_layout_index_t layout = 0; layout < num_layouts; layout++) {
			xkb_level_index_t num_levels = xkb_keymap_num_levels_for_key(xkeymap->keymap, keycode, layout);

			for (xkb_level_index_t level = 0; level < num_levels; level++) {
				xkb_keysym_t sym;
				int ret, value;

				if (!(ret = xkeymap_get_symbol(xkeymap->keymap, keycode, layout, level, &sym)))
					continue;
				else if (ret < 0)
					goto err;

				if (getenv("LK_XKB_DEBUG")) {
					xkeymap_walk_printer(xkeymap, layout, level, keycode, sym);
					continue;
				}

				if (sym == XKB_KEY_ISO_Next_Group) {
					int shiftl_lock = lk_ksym_to_unicode(xkeymap->ctx, "ShiftL_Lock");
					int shiftr_lock = lk_ksym_to_unicode(xkeymap->ctx, "ShiftR_Lock");

					xkeymap_add_value(xkeymap, layout_switch[0], shiftl_lock, keyvalue);
					xkeymap_add_value(xkeymap, layout_switch[1], shiftr_lock, keyvalue);
					xkeymap_add_value(xkeymap, layout_switch[2], shiftr_lock, keyvalue);
					xkeymap_add_value(xkeymap, layout_switch[3], shiftl_lock, keyvalue);

					goto process_keycode;
				}

				if ((value = xkeymap_get_code(xkeymap->ctx, sym)) < 0)
					continue;

				xkeymap_keycode_mask(xkeymap->keymap, layout, level, keycode, &keycode_mask);

				for (unsigned short i = 0; i < ARRAY_SIZE(layout_switch); i++) {
					const struct modifier_mapping *map;
					int modifier = layout_switch[i];

					if (layout != layouts[num_layouts - 1][i])
						continue;

					for (xkb_mod_index_t mod = 0; mod < num_mods; mod++) {
						if (!(keycode_mask.mask[0] & (1u << mod)))
							continue;

						map = convert_modifier(xkb_keymap_mod_get_name(xkeymap->keymap, mod));
						if (map)
							modifier |= map->bit;
					}

					xkeymap_add_value(xkeymap, modifier, value, keyvalue);
				}
			}
		}

		if (getenv("LK_XKB_DEBUG"))
			continue;

process_keycode:
		unsigned short layout = 0;

		for (unsigned short i = 0; i < ARRAY_SIZE(keyvalue); i++) {
			if (layout < ARRAY_SIZE(layout_switch) && i == layout_switch[layout + 1])
				layout++;

			if (!keyvalue[i]) {
				if ((i & (1 << KG_SHIFT)))
					keyvalue[i] = keyvalue[layout_switch[layout] | (1 << KG_SHIFT)];
				if (!keyvalue[i])
					keyvalue[i] = keyvalue[layout_switch[layout]];
			}

			if (lk_add_key(xkeymap->ctx, i, (int) KERN_KEYCODE(keycode), keyvalue[i]) < 0)
				goto err;
		}
	}

	return 0;
err:
	return -1;
}


int convert_xkb_keymap(struct lk_ctx *ctx, struct xkb_rule_names *names, int options)
{
	struct xkeymap xkeymap;
	int ret = -1;

	xkeymap.ctx = ctx;

	lk_set_keywords(ctx, LK_KEYWORD_ALTISMETA | LK_KEYWORD_STRASUSUAL);

	xkeymap.xkb = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	if (!xkeymap.xkb) {
		kbd_warning(0, "xkb_context_new failed");
		goto end;
	}

	xkeymap.keymap = xkb_keymap_new_from_names(xkeymap.xkb, names, XKB_KEYMAP_COMPILE_NO_FLAGS);
	if (!xkeymap.keymap) {
		kbd_warning(0, "xkb_keymap_new_from_names failed");
		goto end;
	}

	if (xkb_keymap_num_layouts(xkeymap.keymap) > ARRAY_SIZE(layout_switch)) {
		kbd_warning(0, "too many layouts specified. At the moment, you can use no more than %ld", ARRAY_SIZE(layout_switch));
		goto end;
	}

	if (!(ret = xkeymap_walk(&xkeymap))) {
		if (options & OPT_P)
			lk_dump_keymap(ctx, stdout, LK_SHAPE_SEPARATE_LINES, 0);
	}

end:
	xkb_keymap_unref(xkeymap.keymap);
	xkb_context_unref(xkeymap.xkb);

	return ret;
}
