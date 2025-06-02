#define _GNU_SOURCE
#include <search.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include "libcommon.h"
#include "keymap.h"
#include "xkbsupport.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

/*
 * The number of layouts that can be processed by this implementation.
 */
#define NR_LAYOUTS 4

static const int layout_switch[NR_LAYOUTS] = {
	(0),
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
static const unsigned int layouts[NR_LAYOUTS][NR_LAYOUTS] = {
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
	const char *debug;
	struct xkb_context *xkb;
	struct xkb_keymap *keymap;
	struct xkb_compose_table *compose;
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

/*
 * From xkbcommon documentation:
 *
 * The following table lists the usual modifiers present in the [standard
 * keyboard configuration][xkeyboard-config]. Note that this is provided for
 * information only, as it may change depending on the user configuration.
 *
 * | Modifier     | Type    | Usual mapping    | Comment                             |
 * | ------------ | ------- | ---------------- | ----------------------------------- |
 * | `Shift`      | Real    | `Shift`          | The usual [Shift]                   |
 * | `Lock`       | Real    | `Lock`           | The usual [Caps Lock][Lock]         |
 * | `Control`    | Real    | `Control`        | The usual [Control]                 |
 * | `Mod1`       | Real    | `Mod1`           | Not conventional                    |
 * | `Mod2`       | Real    | `Mod2`           | Not conventional                    |
 * | `Mod3`       | Real    | `Mod3`           | Not conventional                    |
 * | `Mod4`       | Real    | `Mod4`           | Not conventional                    |
 * | `Mod5`       | Real    | `Mod5`           | Not conventional                    |
 * | `Alt`        | Virtual | `Mod1`           | The usual [Alt]                     |
 * | `Meta`       | Virtual | `Mod1` or `Mod4` | The legacy [Meta] key               |
 * | `NumLock`    | Virtual | `Mod2`           | The usual [NumLock]                 |
 * | `Super`      | Virtual | `Mod4`           | The usual [Super]/GUI               |
 * | `LevelThree` | Virtual | `Mod5`           | [ISO][ISO9995] level 3, aka [AltGr] |
 * | `LevelFive`  | Virtual | `Mod3`           | [ISO][ISO9995] level 5              |
 *
 * [Shift]: https://en.wikipedia.org/wiki/Control_key
 * [Lock]: https://en.wikipedia.org/wiki/Caps_Lock
 * [Control]: https://en.wikipedia.org/wiki/Control_key
 * [Alt]: https://en.wikipedia.org/wiki/Alt_key
 * [AltGr]: https://en.wikipedia.org/wiki/AltGr_key
 * [NumLock]: https://en.wikipedia.org/wiki/Num_Lock
 * [Meta]: https://en.wikipedia.org/wiki/Meta_key
 * [Super]: https://en.wikipedia.org/wiki/Super_key_(keyboard_button)
 *
 * See: https://github.com/xkbcommon/libxkbcommon/blob/master/doc/keymap-format-text-v1.md
 */
struct modifier_mapping {
	const char *xkb_mod;
	const char *krn_mod;
	const int bit;
};

static struct modifier_mapping modifier_mapping[] = {
	{ "Shift",		"shift",	(1u << KG_SHIFT)	},
	{ "Lock",		"capslock",	(1u << KG_CAPSSHIFT)	},
	{ "Control",		"control",	(1u << KG_CTRL)		},
	{ "Mod1",		"alt",		(1u << KG_ALT)		},
	{ "Mod2",		"<numlock>",	0			},
	{ "Mod3",		"alt",		(1u << KG_ALT)		},
	{ "Mod4",		"<super>",	0			},
	{ "Mod5",		"altgr",	(1u << KG_ALTGR)	},
	{ "Alt",		"alt",		(1u << KG_ALT)		},
	{ "Meta",		"<meta>",	0			},
	{ "NumLock",		"<numlock>",	0			},
	{ "Super",		"<super>",	0			},
	{ "LevelThree",		"altgr",	(1u << KG_ALTGR)	},
	{ "LevelFive",		"alt",		(1u << KG_ALT)		},
	{ NULL,			NULL,		0			},
};

static int is_debug(struct xkeymap *xkeymap, const char *val)
{
	if (!xkeymap->debug || (val && strcmp(xkeymap->debug, val)))
		return 0;
	return 1;
}

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
	int padding = 30;
	xkb_mod_index_t num_mods = xkb_keymap_num_mods(keymap);

	for (size_t m = 0; m < mask->num; m++) {
		for (xkb_mod_index_t mod = 0; mod < num_mods; mod++) {
			if ((mask->mask[m] & (UINT32_C(1) << mod)) == 0)
				continue;

			const char *modname = xkb_keymap_mod_get_name(keymap, mod);

			padding -= printf(" %ld:%s(%d)", m, modname, mod);
		}
	}

	if (padding == 30)
		padding -= printf(" -");

	while (padding-- > 0)
		putchar(' ');
}

static void xkeymap_keycode_mask(struct xkb_keymap *keymap,
                                 xkb_layout_index_t layout, xkb_level_index_t level, xkb_keycode_t keycode,
                                 struct xkb_mask *out)
{
	memset(out->mask, 0, sizeof(out->mask));
	out->num = xkb_keymap_key_get_mods_for_level(keymap, keycode, layout, level, out->mask, ARRAY_SIZE(out->mask));
}

static int parse_hexcode(struct lk_ctx *ctx, const char *symname)
{
	int value;
	size_t ret = strlen(symname);

	if (ret >= 4 && symname[0] == '0' && symname[1] == 'x') {
		errno = 0;
		value = (int) strtol(symname + 2, NULL, 16);
		if (errno == ERANGE) {
			kbd_warning(0, "unable to convert unnamed non-Unicode xkb symbol `%s'", symname);
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
			kbd_warning(0, "unable to convert unnamed unicode xkb symbol `%s'", symname);
			return -1;
		}
		return lk_convert_code(ctx, value ^ 0xf000, TO_UNICODE);
	}

	return 0;
}

static int xkeymap_get_code(struct xkeymap *xkeymap, xkb_keysym_t symbol)
{
	uint32_t xkb_unicode;
	int ret;
	const char *symname;
	char symbuf[BUFSIZ];

	symbuf[0] = 0;

	ret = xkb_keysym_get_name(symbol, symbuf, sizeof(symbuf));

	if (ret < 0 || (size_t)ret >= sizeof(symbuf)) {
		kbd_warning(0, "failed to get name of keysym");
		return -1;
	}

	/*
	 * First. Let's try to translate the xkb name into the kbd name.
	 */
	if ((symname = map_xkbsym_to_ksym(xkeymap, symbuf)) != NULL &&
	    lk_valid_ksym(xkeymap->ctx, symname, TO_UNICODE))
		ret = lk_ksym_to_unicode(xkeymap->ctx, symname);

	/*
	 * Second. If the symbol name is known to us, that is, it matches
	 * the kbd being used, then we use it.
	 */
	else if (lk_valid_ksym(xkeymap->ctx, symbuf, TO_UNICODE))
		ret = lk_ksym_to_unicode(xkeymap->ctx, symbuf);

	/*
	 * Third. Let's try to use utf32 code.
	 */
	else if ((xkb_unicode = xkb_keysym_to_utf32(symbol)) > 0)
		ret = (int) (xkb_unicode ^ 0xf000);

	/*
	 * Last chance.
	 */
	else
		ret = parse_hexcode(xkeymap->ctx, symbuf);

	/*
	 * The kbentry.kb_value is unsigned short.
	 */
	if (ret <= 0 || ret > USHRT_MAX)
		ret = -1;

	return ret;
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

	printf("keycode %3d = ", KERN_KEYCODE(keycode));

	printf("layout[%d]= %-12s level= %d",
		layout, xkb_keymap_layout_get_name(xkeymap->keymap, layout), level);

	char s[BUFSIZ];
	const char *symname = NULL;
	int value, ret;

	ret = xkb_keysym_get_name(sym, s, sizeof(s));

	if (ret < 0 || (size_t)ret >= sizeof(s)) {
		kbd_warning(0, "failed to get name of keysym");
		return;
	}

	struct xkb_mask keycode_mask;
	xkeymap_keycode_mask(xkeymap->keymap, layout, level, keycode, &keycode_mask);

	printf(" xkb:symname= %-27s xkb:mods=", s);
	print_modifiers(xkeymap->keymap, &keycode_mask);
	printf(" ");

	if (lk_valid_ksym(xkeymap->ctx, s, TO_UNICODE))
		symname = s;
	else
		symname = map_xkbsym_to_ksym(xkeymap, s);

	if (!symname)
		symname = "-";

	value = xkeymap_get_code(xkeymap, sym);

	printf(" kbd:code= U+%04x kbd:symname= %-32s", value, symname);

	int kbd_mods = 0;

	printf(" kbd:mods=");
	if (keycode_mask.num > 0) {
		xkb_mod_index_t num_mods = xkb_keymap_num_mods(xkeymap->keymap);

		for (xkb_mod_index_t mod = 0; mod < num_mods; mod++) {
			if ((keycode_mask.mask[0] & (1u << mod)) == 0)
				continue;

			const struct modifier_mapping *map = convert_modifier(xkb_keymap_mod_get_name(xkeymap->keymap, mod));

			printf(" %s", map->krn_mod);
			kbd_mods++;
		}
	}
	if (!kbd_mods)
		printf(" -");
	printf("\n");

	fflush(stdout);
	fflush(stderr);
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

static int get_kernel_modifier(struct xkeymap *xkeymap, xkb_mod_mask_t xkbmask)
{
	const struct modifier_mapping *map;
	int modifier = 0;
	xkb_mod_index_t num_mods = xkb_keymap_num_mods(xkeymap->keymap);

	for (xkb_mod_index_t mod = 0; mod < num_mods; mod++) {
		if (!(xkbmask & (UINT32_C(1) << mod)))
			continue;

		map = convert_modifier(xkb_keymap_mod_get_name(xkeymap->keymap, mod));
		if (map)
			modifier |= map->bit;
	}

	return modifier;
}

static int xkeymap_walk(struct xkeymap *xkeymap)
{
	struct xkb_mask keycode_mask;
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

	int shift       = lk_ksym_to_unicode(xkeymap->ctx, "Shift");
	int shiftl      = lk_ksym_to_unicode(xkeymap->ctx, "ShiftL");
	int shiftr      = lk_ksym_to_unicode(xkeymap->ctx, "ShiftR");
	int shift_lock  = lk_ksym_to_unicode(xkeymap->ctx, "Shift_Lock");
	int shiftl_lock = lk_ksym_to_unicode(xkeymap->ctx, "ShiftL_Lock");
	int shiftr_lock = lk_ksym_to_unicode(xkeymap->ctx, "ShiftR_Lock");

	xkb_layout_index_t num_layouts = xkb_keymap_num_layouts(xkeymap->keymap);

	/*
	 * Pick the languages layout. The switching order depends on the
	 * number of languages (xkb layouts).
	 */
	const unsigned int *kmap_layout = layouts[num_layouts - 1];

	for (xkb_keycode_t keycode = min_keycode; keycode <= max_keycode; keycode++) {
		int keyvalue[MAX_NR_KEYMAPS];

		memset(keyvalue, 0, sizeof(keyvalue));

		/*
		 * A mapping of keycodes to symbols, actions and key types.
		 *
		 * A user who deals with multiple languages may need two or more
		 * different layouts: e.g. a layout for Arabic and another one for
		 * English. In this context, layouts are called _groups_ in XKB,
		 * as defined in the [standard ISO/IEC&nbsp;9995][ISO9995].
		 *
		 * Layouts are ordered and identified by their index. Example:
		 *
		 * - Layout 1: Arabic
		 * - Layout 2: English
		 *
		 * See: https://github.com/xkbcommon/libxkbcommon/blob/master/doc/keymap-format-text-v1.md
		 */
		for (xkb_layout_index_t layout = 0; layout < num_layouts; layout++) {
			xkb_level_index_t num_levels = xkb_keymap_num_levels_for_key(xkeymap->keymap, keycode, layout);

			/*
			 * A key type defines the levels available for a key and
			 * how to derive the active level from the modifiers states. Examples:
			 * - `ONE_LEVEL`: the key has only one level, i.e. it is not affected
			 *    by any modifiers. Example: the modifiers themselves.
			 * - `TWO_LEVEL`: the key has two levels:
			 *   - Level 1: default level, active when the `Shift` modifier is _not_ active.
			 *   - Level 2: level activated with the `Shift` modifier.
			 * - `FOUR_LEVEL`: see the example in the previous section.
			 *
			 * See: https://github.com/xkbcommon/libxkbcommon/blob/master/doc/keymap-format-text-v1.md
			*/
			for (xkb_level_index_t level = 0; level < num_levels; level++) {
				xkb_keysym_t sym;
				int ret, value, modifier;

				/*
				 * In XKB world, a key action defines the effect a key
				 * has on the state of the keyboard or the state of the display server.
				 * Examples:
				 *
				 * - Change the state of a modifier.
				 * - Change the active group.
				 * - Move the mouse pointer.
				 *
				 * See: https://github.com/xkbcommon/libxkbcommon/blob/master/doc/keymap-format-text-v1.md
				 */
				if (!(ret = xkeymap_get_symbol(xkeymap->keymap, keycode, layout, level, &sym)))
					continue;
				else if (ret < 0)
					goto err;

				if (is_debug(xkeymap, NULL))
					xkeymap_walk_printer(xkeymap, layout, level, keycode, sym);

				xkeymap_keycode_mask(xkeymap->keymap, layout, level, keycode, &keycode_mask);

				for (size_t m = 0; m < keycode_mask.num; m++) {
					modifier = get_kernel_modifier(xkeymap, keycode_mask.mask[m]);

					if (sym == XKB_KEY_ISO_Next_Group) {
						xkeymap_add_value(xkeymap, modifier | layout_switch[0], shiftl_lock, keyvalue);
						xkeymap_add_value(xkeymap, modifier | layout_switch[1], shiftr_lock, keyvalue);
						xkeymap_add_value(xkeymap, modifier | layout_switch[2], shiftr_lock, keyvalue);
						xkeymap_add_value(xkeymap, modifier | layout_switch[3], shiftl_lock, keyvalue);

						goto process_keycode;
					}

					if ((value = xkeymap_get_code(xkeymap, sym)) < 0)
						continue;

					/*
					* Replace ShiftL/ShiftR by Shift to protect
					* layout switches.
					*/
					if (value == shiftl ||
					value == shiftr)
						value = shift;

					if (value == shiftl_lock ||
					value == shiftr_lock)
						value = shift_lock;

					for (unsigned short i = 0; i < NR_LAYOUTS; i++) {
						if (layout != kmap_layout[i])
							continue;
						xkeymap_add_value(xkeymap, modifier | layout_switch[i], value, keyvalue);
					}
				}
			}
		}

process_keycode:
		for (unsigned short i = 0; i < ARRAY_SIZE(keyvalue); i++) {
			if (!keyvalue[i])
				continue;
			if (lk_add_key(xkeymap->ctx, i, (int) KERN_KEYCODE(keycode), keyvalue[i]) < 0)
				goto err;
			remember_code(xkeymap, keyvalue[i]);
		}
	}

	return 0;
err:
	return -1;
}

static int parsemap(struct xkeymap *xkeymap, FILE *fp, const char *filename)
{
	char buffer[BUFSIZ];
	char *p, *q, *xkb_sym = NULL, *krn_sym = NULL;
	struct sym_pair **val, *pair = NULL;
	int line_nr = 0;

	while (fgets(buffer, sizeof(buffer) - 1, fp)) {
		line_nr++;

		buffer[BUFSIZ - 1] = '\0';

		if (!(p = strchr(buffer, '\n'))) {
			kbd_warning(0, "%s:%d: The line does not end with a newline. Looks like the line is too long.",
			            filename, line_nr);
			goto err;
		}
		*p = '\0';

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

static void xkeymap_compose_printer(struct xkb_compose_table_entry *entry)
{
	char buf[128];
	int offset = 20;
	size_t seqlen = 0;
	const xkb_keysym_t *syms = xkb_compose_table_entry_sequence(entry, &seqlen);
	const char *chr = xkb_compose_table_entry_utf8(entry);
	xkb_keysym_t keysym = xkb_compose_table_entry_keysym(entry);

	printf("Compose: \"%s\" (chars=%ld) ", chr, strlen(chr));

	if (xkb_keysym_get_name(keysym, buf, sizeof(buf)) > 0)
		offset -= printf("<%s>", buf);
	else
		offset -= printf("<?>");

	for (; offset > 0; offset--)
		printf(" ");

	printf(" -> sequence[%ld] = { ", seqlen);
	for (size_t i = 0; i < seqlen; i++) {
		if (xkb_keysym_get_name(syms[i], buf, sizeof(buf)) > 0)
			printf("<%s> ", buf);
		else
			printf("<?> ");
	}
	printf("}\n");
}

static int xkeymap_compose(struct xkeymap *xkeymap)
{
	int count, ret;
	struct xkb_compose_table_entry *entry;
	struct xkb_compose_table_iterator *iter = xkb_compose_table_iterator_new(xkeymap->compose);

	if (!iter) {
		kbd_warning(0, "xkb_compose_table_iterator_new failed");
		return -1;
	}

	count = ret = 0;

	while ((entry = xkb_compose_table_iterator_next(iter))) {
		size_t seqlen = 0;
		const xkb_keysym_t *syms = xkb_compose_table_entry_sequence(entry, &seqlen);
		xkb_keysym_t keysym = xkb_compose_table_entry_keysym(entry);

		if (is_debug(xkeymap, "1")) {
			xkeymap_compose_printer(entry);
			continue;
		}

		if (keysym == XKB_KEY_NoSymbol)
			continue;

		if (seqlen == 2) {
			struct lk_kbdiacr ptr;
			int code;

			if ((code = xkeymap_get_code(xkeymap, syms[0])) < 0 ||
			    !used_code(xkeymap, code))
				continue;

			ptr.diacr = (unsigned int) code;

			if ((code = xkeymap_get_code(xkeymap, syms[1])) < 0 ||
			    !used_code(xkeymap, code))
				continue;

			ptr.base = (unsigned int) code;

			if ((code = xkeymap_get_code(xkeymap, keysym)) < 0 ||
			    used_code(xkeymap, code))
				continue;

			ptr.result = (unsigned int) code;

			if (++count >= MAX_DIACR)
				continue;

			if (is_debug(xkeymap, "2")) {
				xkeymap_compose_printer(entry);
				continue;
			}

			ret = lk_append_diacr(xkeymap->ctx, &ptr);

			if (ret == -1)
				break;
		}
	}

	xkb_compose_table_iterator_free(iter);

	if (count >= MAX_DIACR)
		kbd_warning(0, "kernel can handle only %d compose definitions, but xkb specified %d.",
		            MAX_DIACR, count);

	return ret;
}

static int load_translation_table(struct xkeymap *xkeymap)
{
	const char *filename = DATADIR "/xkbtrans/names";
	FILE *fp;

	fp = fopen(filename, "r");
	if (!fp) {
		filename = getenv("LK_XKB_TRANSLATION_TABLE");

		if (filename)
			fp = fopen(filename, "r");

		if (!fp) {
			kbd_warning(0, "%s: translation table not found. Use the LK_XKB_TRANSLATION_TABLE environment variable to specify this file.",
			            filename);
			return -1;
		}
	}

	if (parsemap(xkeymap, fp, filename) < 0) {
		kbd_warning(0, "unable to parse xkb translation names");
		return -1;
	}
	fclose(fp);

	return 0;
}

int convert_xkb_keymap(struct lk_ctx *ctx, struct xkeymap_params *params)
{
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
	xkeymap.debug = getenv("LK_XKB_DEBUG");

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

	if (xkb_keymap_num_layouts(xkeymap.keymap) > NR_LAYOUTS) {
		kbd_warning(0, "too many layouts specified. At the moment, you can use no more than %d", NR_LAYOUTS);
		goto end;
	}

	if (params->locale) {
		xkeymap.compose = xkb_compose_table_new_from_locale(xkeymap.xkb, params->locale, XKB_COMPOSE_COMPILE_NO_FLAGS);
		if (!xkeymap.compose) {
			kbd_warning(0, "xkb_compose_table_new_from_locale failed");
			goto end;
		}
	}

	if (load_translation_table(&xkeymap) < 0)
		 goto end;

	if (xkeymap_walk(&xkeymap) < 0)
		goto end;

	if (xkeymap.compose && xkeymap_compose(&xkeymap) < 0)
		goto end;

	ret = 0;
end:
	if (xkeymap.used_codes)
		tdestroy(xkeymap.used_codes, free);

	if (xkeymap.syms_map)
		tdestroy(xkeymap.syms_map, xkeymap_pair_free);

	if (xkeymap.compose)
		xkb_compose_table_unref(xkeymap.compose);

	xkb_keymap_unref(xkeymap.keymap);
	xkb_context_unref(xkeymap.xkb);

	return ret;
}
