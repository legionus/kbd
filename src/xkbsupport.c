#define _GNU_SOURCE
#include <search.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

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

struct sym_pair {
	char *xkb_sym;
	char *krn_sym;
};

struct xkeymap {
	struct xkb_context *xkb;
	struct xkb_keymap *keymap;
	struct lk_ctx *ctx;
	void *used_codes;
	void *syms_map;
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

static void xkeymap_pair_free(void *pa)
{
	struct sym_pair *pair = pa;
	if (!pair)
		return;
	free(pair->xkb_sym);
	free(pair->krn_sym);
	free(pair);
}

static int compare_by_xkbsym(const void *pa, const void *pb)
{
	return strcmp(((struct sym_pair *)pa)->xkb_sym,
		      ((struct sym_pair *)pb)->xkb_sym);
}

static const char *map_xkbsym_to_ksym(struct xkeymap *xkeymap, char *xkb_sym)
{
	struct sym_pair **val;
	struct sym_pair pair = {
		.xkb_sym = xkb_sym,
	};

	val = tfind(&pair, &xkeymap->syms_map, compare_by_xkbsym);
	if (val)
		return ((struct sym_pair *) *val)->krn_sym;
	return NULL;
}

static int compare_codes(const void *pa, const void *pb)
{
	if (*(int *) pa < *(int *) pb)
		return -1;
	if (*(int *) pa > *(int *) pb)
		return 1;
	return 0;
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

	if (!(symname = map_xkbsym_to_ksym(xkeymap, s)))
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

static int parse_hexcode(const char *symname)
{
	int unicode;
	size_t ret = strlen(symname);

	if (ret >= 4 && symname[0] == '0' && symname[1] == 'x') {
		errno = 0;
		unicode = (int) strtol(symname + 2, NULL, 16);
		if (errno == ERANGE) {
			kbd_warning(0, "unable to convert unnamed non-Unicode xkb symbol `%s'", symname);
			return -1;
		}
		return unicode;
	}

	if (ret >= 5 && symname[0] == 'U' &&
	   isxdigit(symname[1]) && isxdigit(symname[2]) &&
	   isxdigit(symname[3]) && isxdigit(symname[4])) {
		errno = 0;
		unicode = (int) strtol(symname + 1, NULL, 16);
		if (errno == ERANGE) {
			kbd_warning(0, "unable to convert unnamed unicode xkb symbol `%s'", symname);
			return -1;
		}
		return unicode;
	}

	return 0;
}

static int xkeymap_get_code(struct xkeymap *xkeymap, xkb_keysym_t symbol)
{
	uint32_t xkb_unicode;
	int ret, unicode;
	const char *symname;
	char symbuf[BUFSIZ];

	symbuf[0] = 0;

	ret = xkb_keysym_get_name(symbol, symbuf, sizeof(symbuf));

	if (ret < 0 || (size_t)ret >= sizeof(symbuf)) {
		kbd_warning(0, "failed to get name of keysym");
		return K_HOLE;
	}

	if ((unicode = parse_hexcode(symbuf)) != 0) {
		;
	} else if ((symname = map_xkbsym_to_ksym(xkeymap, symbuf)) != NULL) {
		unicode = parse_hexcode(symname);
		if (!unicode)
			unicode = lk_ksym_to_unicode(xkeymap->ctx, symname);

	} else if ((xkb_unicode = xkb_keysym_to_utf32(symbol)) > 0) {
		unicode = (int) xkb_unicode;
	} else {
		unicode = K_HOLE;
	}

	ret = CODE_FOR_UNKNOWN_KSYM;

	if (unicode > 0)
		ret = lk_convert_code(xkeymap->ctx, unicode, TO_AUTO);

	if (ret == CODE_FOR_UNKNOWN_KSYM) {
		kbd_warning(0, "unable to convert code <%s> and replace it with VoidSymbol", symbuf);
		ret = K_HOLE;
	}

	return ret;
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

static int used_code(struct xkeymap *xkeymap, int code)
{
	return tfind(&code, &xkeymap->used_codes, compare_codes) != NULL;
}

static void remember_code(struct xkeymap *xkeymap, int code)
{
	int *ptr, **val;

	if (used_code(xkeymap, code))
		return;

	ptr = malloc(sizeof(code));
	if (!ptr)
		kbd_error(1, errno, "out of memory");

	*ptr = code;

	val = tsearch(ptr, &xkeymap->used_codes, compare_codes);
	if (!val)
		kbd_error(1, errno, "out of memory");

	if (*val != ptr)
		free(ptr);
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

				if ((value = xkeymap_get_code(xkeymap, sym)) < 0)
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

			remember_code(xkeymap, keyvalue[i]);

			if (getenv("LK_XKB_DEBUG"))
				continue;

			if (lk_add_key(xkeymap->ctx, i, (int) KERN_KEYCODE(keycode), keyvalue[i]) < 0)
				goto err;
		}
	}

	return 0;
err:
	return -1;
}

static int parsemap(struct xkeymap *xkeymap, FILE *fp)
{
	char buffer[BUFSIZ];
	char *p, *q, *xkb_sym = NULL, *krn_sym = NULL;
	struct sym_pair **val, *pair = NULL;

	while (fgets(buffer, sizeof(buffer) - 1, fp)) {
		for (p = buffer; isspace(*p); p++);

		if (*p == '#' || *p == '\0')
			continue;

		for (q = p; *q != '\0' && !isspace(*q); q++);
		*q = '\0';

		xkb_sym = p;

		if (p != q)
			for(p = ++q; isspace(*p); p++);

		for (q = p; *q != '\0' && !isspace(*q); q++);
		*q = '\0';

		krn_sym = p;

		pair = calloc(1, sizeof(*pair));
		if (!pair)
			goto err;

		pair->xkb_sym = strdup(xkb_sym);
		pair->krn_sym = strdup(krn_sym);

		if (!pair->xkb_sym || !pair->krn_sym)
			goto err;

		val = tsearch(pair, &xkeymap->syms_map, compare_by_xkbsym);
		if (!val)
			goto err;

		if (*val != pair)
			xkeymap_pair_free(pair);

		pair = NULL;
	}
	return 0;
err:
	xkeymap_pair_free(pair);
	return -1;
}

int convert_xkb_keymap(struct lk_ctx *ctx, struct xkeymap_params *params, int options)
{
	FILE *fp;
	struct xkeymap xkeymap = { 0 };
	int ret = -1;

	struct xkb_rule_names names = {
		.rules = "evdev",
		.model = params->model,
		.layout = params->layout,
		.variant = params->variant,
		.options = params->options,
	};

	xkeymap.ctx = ctx;

	lk_set_keywords(ctx, LK_KEYWORD_ALTISMETA | LK_KEYWORD_STRASUSUAL);

	xkeymap.xkb = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	if (!xkeymap.xkb) {
		kbd_warning(0, "xkb_context_new failed");
		goto end;
	}

	xkeymap.keymap = xkb_keymap_new_from_names(xkeymap.xkb, &names, XKB_KEYMAP_COMPILE_NO_FLAGS);
	if (!xkeymap.keymap) {
		kbd_warning(0, "xkb_keymap_new_from_names failed");
		goto end;
	}

	if (xkb_keymap_num_layouts(xkeymap.keymap) > ARRAY_SIZE(layout_switch)) {
		kbd_warning(0, "too many layouts specified. At the moment, you can use no more than %ld", ARRAY_SIZE(layout_switch));
		goto end;
	}

	if ((fp = fopen(DATADIR "/xkbtrans/names", "r")) != NULL) {
		if (parsemap(&xkeymap, fp) < 0) {
			kbd_warning(0, "unable to parse xkb translation names");
			goto end;
		}
		fclose(fp);
	}

	if (!(ret = xkeymap_walk(&xkeymap))) {
		if (options & OPT_P)
			lk_dump_keymap(ctx, stdout, LK_SHAPE_SEPARATE_LINES, 0);
	}

end:
	if (xkeymap.used_codes)
		tdestroy(xkeymap.used_codes, free);

	if (xkeymap.syms_map)
		tdestroy(xkeymap.syms_map, xkeymap_pair_free);

	xkb_keymap_unref(xkeymap.keymap);
	xkb_context_unref(xkeymap.xkb);

	return ret;
}
