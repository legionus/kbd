#define _GNU_SOURCE
#include <search.h>

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include "libcommon.h"
#include "keymap.h"
#include "xkbsupport.h"

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

struct reachable_sym {
	xkb_keysym_t sym;
	int code;
};

struct xkeymap {
	struct xkb_context *xkb;
	struct xkb_keymap *keymap;
	struct xkb_compose_table *compose;
	struct lk_ctx *ctx;
	void *reachable_syms;
	void *syms_map;
};

struct compose_candidate {
	struct lk_kbdiacr diacr;
	xkb_keysym_t seq[2];
	xkb_keysym_t result_sym;
	int result_reachable;
	unsigned int score;
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
 * | `LevelThree` | Virtual | `Mod3`           | [ISO][ISO9995] level 3, aka [AltGr] |
 * | `LevelFive`  | Virtual | `Mod5`           | [ISO][ISO9995] level 5              |
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
	{ "Mod3",		"altgr",	(1u << KG_ALTGR)	},
	{ "Mod4",		"<super>",	0			},
	{ "Mod5",		"alt",		(1u << KG_ALT)		},
	{ "Alt",		"alt",		(1u << KG_ALT)		},
	{ "Meta",		"<meta>",	0			},
	{ "NumLock",		"<numlock>",	0			},
	{ "Super",		"<super>",	0			},
	{ "LevelThree",		"altgr",	(1u << KG_ALTGR)	},
	{ "LevelFive",		"alt",		(1u << KG_ALT)		},
	{ NULL,			NULL,		0			},
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

static int compare_reachable_syms(const void *pa, const void *pb)
{
	const struct reachable_sym *lhs = pa;
	const struct reachable_sym *rhs = pb;

	if (lhs->sym < rhs->sym)
		return -1;
	if (lhs->sym > rhs->sym)
		return 1;
	return 0;
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

static void xkeymap_keycode_mask(struct xkb_keymap *keymap,
                                 xkb_layout_index_t layout, xkb_level_index_t level, xkb_keycode_t keycode,
                                 struct xkb_mask *out)
{
	size_t i, j;

	memset(out->mask, 0, sizeof(out->mask));
	out->num = xkb_keymap_key_get_mods_for_level(keymap, keycode, layout, level, out->mask, ARRAY_SIZE(out->mask));

	/*
	 * XKB may expose several masks for the same level.  Prefer the
	 * simplest one so the resulting kernel table stays deterministic.
	 */
	for (i = 0; i < out->num; i++) {
		for (j = i + 1; j < out->num; j++) {
			unsigned int lhs = xkb_mask_weight(out->mask[i]);
			unsigned int rhs = xkb_mask_weight(out->mask[j]);

			if (rhs < lhs || (rhs == lhs && out->mask[j] < out->mask[i])) {
				xkb_mod_mask_t tmp = out->mask[i];
				out->mask[i] = out->mask[j];
				out->mask[j] = tmp;
			}
		}
	}
}

static int parse_hexcode(struct lk_ctx *ctx, const char *symname)
{
	int value;
	size_t ret = strlen(symname);

	if (ret >= 4 && symname[0] == '0' && symname[1] == 'x') {
		errno = 0;
		value = (int) strtol(symname + 2, NULL, 16);
		if (errno == ERANGE) {
			XKEYMAP_WARNING(0, "unable to convert unnamed non-Unicode xkb symbol `%s'", symname);
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
			XKEYMAP_WARNING(0, "unable to convert unnamed unicode xkb symbol `%s'", symname);
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
		XKEYMAP_WARNING(0, "failed to get name of keysym");
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

static int xkeymap_get_symbol(struct xkb_keymap *keymap,
		xkb_keycode_t keycode, xkb_layout_index_t layout, xkb_level_index_t level,
		xkb_keysym_t *symbol)
{
	int num_syms;
	const xkb_keysym_t *syms = NULL;

	if (!(num_syms = xkb_keymap_key_get_syms_by_level(keymap, keycode, layout, level, &syms)))
		return 0;

	if (num_syms > 1) {
		XKEYMAP_WARNING(0, "keycode %u layout %u level %u has %d symbols; using the first one",
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
		kbd_error(1, errno, "out of memory");

	ptr->sym = sym;
	ptr->code = code;

	val = tsearch(ptr, &xkeymap->reachable_syms, compare_reachable_syms);
	if (!val)
		kbd_error(1, errno, "out of memory");

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

static void xkeymap_add_value(struct xkeymap *xkeymap, int modifier, int code,
			      int capslockable, int keyvalue[MAX_NR_KEYMAPS])
{
	if (modifier < 0 || modifier >= MAX_NR_KEYMAPS)
		return;

	if (capslockable && (!modifier || (modifier & (1 << KG_SHIFT))))
		code = lk_add_capslock(xkeymap->ctx, code);

	if (keyvalue[modifier])
		return;

	keyvalue[modifier] = code;
}

static int get_kernel_modifier(struct xkeymap *xkeymap, xkb_mod_mask_t xkbmask, int *modifier)
{
	const struct modifier_mapping *map;
	int ret = 0;
	xkb_mod_index_t num_mods = xkb_keymap_num_mods(xkeymap->keymap);

	*modifier = 0;

	for (xkb_mod_index_t mod = 0; mod < num_mods; mod++) {
		if (!(xkbmask & xkb_mod_bit(mod)))
			continue;

		map = convert_modifier(xkb_keymap_mod_get_name(xkeymap->keymap, mod));
		/* Ignore XKB states that have no safe kernel-table representation. */
		if (!map || !map->bit)
			return -1;

		*modifier |= map->bit;
		ret = 1;
	}

	return ret;
}

static int xkeymap_walk(struct xkeymap *xkeymap)
{
	struct xkb_mask keycode_mask;
	xkb_keycode_t min_keycode = xkb_keymap_min_keycode(xkeymap->keymap);
	xkb_keycode_t max_keycode = xkb_keymap_max_keycode(xkeymap->keymap);

	if (KERN_KEYCODE(min_keycode) >= NR_KEYS) {
		XKEYMAP_WARNING(0, "keymap defines more keycodes than the kernel can handle.");
		min_keycode = (NR_KEYS - 1) + EVDEV_OFFSET;
	}

	if (KERN_KEYCODE(max_keycode) >= NR_KEYS) {
		XKEYMAP_WARNING(0, "keymap defines more keycodes than the kernel can handle.");
		max_keycode = (NR_KEYS - 1) + EVDEV_OFFSET;
	}

	int shift       = lk_ksym_to_unicode(xkeymap->ctx, "Shift");
	int shiftl      = lk_ksym_to_unicode(xkeymap->ctx, "ShiftL");
	int shiftr      = lk_ksym_to_unicode(xkeymap->ctx, "ShiftR");
	int shift_lock  = lk_ksym_to_unicode(xkeymap->ctx, "Shift_Lock");
	int shiftl_lock = lk_ksym_to_unicode(xkeymap->ctx, "ShiftL_Lock");
	int shiftr_lock = lk_ksym_to_unicode(xkeymap->ctx, "ShiftR_Lock");

	xkb_layout_index_t num_layouts = xkb_keymap_num_layouts(xkeymap->keymap);

	if (num_layouts == 0 || num_layouts > NR_LAYOUTS) {
		XKEYMAP_WARNING(0, "unable to convert XKB layouts: unsupported layout count %u.",
				(unsigned int) num_layouts);
		return -1;
	}

	/*
	 * Pick the languages layout. The switching order depends on the
	 * number of languages (xkb layouts).
	 */
	const unsigned int *kmap_layout = layouts[num_layouts - 1];

	for (xkb_keycode_t keycode = min_keycode; keycode <= max_keycode; keycode++) {
		int keyvalue[MAX_NR_KEYMAPS];

		memset(keyvalue, 0, sizeof(keyvalue));

		if (xkb_keymap_num_layouts_for_key(xkeymap->keymap, keycode) == 0)
			continue;

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
		/*
		 * Iterate over all layouts in the keymap, not just the layouts
		 * explicitly defined for this key. XKB normalizes out-of-range
		 * per-key layout indexes back into range for these queries, so
		 * walking all global layouts is required to preserve group
		 * fallback semantics in the kernel table.
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
				int ret, value;

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

				xkeymap_keycode_mask(xkeymap->keymap, layout, level, keycode, &keycode_mask);

				if (sym == XKB_KEY_ISO_Next_Group) {
					for (size_t mask_index = 0; mask_index < keycode_mask.num; mask_index++) {
						int modifier;

						if (get_kernel_modifier(xkeymap, keycode_mask.mask[mask_index], &modifier) < 0)
							continue;

						xkeymap_add_value(xkeymap, modifier | layout_switch[0], shiftl_lock, 0, keyvalue);
						xkeymap_add_value(xkeymap, modifier | layout_switch[1], shiftr_lock, 0, keyvalue);
						xkeymap_add_value(xkeymap, modifier | layout_switch[2], shiftr_lock, 0, keyvalue);
						xkeymap_add_value(xkeymap, modifier | layout_switch[3], shiftl_lock, 0, keyvalue);
					}

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

				remember_reachable_sym(xkeymap, sym, value);

				for (size_t mask_index = 0; mask_index < keycode_mask.num; mask_index++) {
					int modifier;

					if (get_kernel_modifier(xkeymap, keycode_mask.mask[mask_index], &modifier) < 0)
						continue;

					for (unsigned short i = 0; i < NR_LAYOUTS; i++) {
						if (layout != kmap_layout[i])
							continue;
						xkeymap_add_value(xkeymap, modifier | layout_switch[i], value,
								 xkeymap_is_capslockable(sym, value), keyvalue);
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
			XKEYMAP_WARNING(0, "%s:%d: The line does not end with a newline. Looks like the line is too long.",
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
			XKEYMAP_WARNING(errno, "out of memory");
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

static unsigned int xkeymap_score_compose_candidate(const struct compose_candidate *candidate)
{
	uint32_t base_unicode = xkeymap_compose_code_to_unicode(candidate->diacr.base);
	uint32_t result_unicode = xkeymap_compose_result_unicode(candidate);
	unsigned int score = 0;

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

static int compare_compose_candidates(const void *pa, const void *pb)
{
	const struct compose_candidate *lhs = pa;
	const struct compose_candidate *rhs = pb;

	if (lhs->score > rhs->score)
		return -1;
	if (lhs->score < rhs->score)
		return 1;
	if (lhs->seq[0] < rhs->seq[0])
		return -1;
	if (lhs->seq[0] > rhs->seq[0])
		return 1;
	if (lhs->seq[1] < rhs->seq[1])
		return -1;
	if (lhs->seq[1] > rhs->seq[1])
		return 1;
	if (lhs->result_sym < rhs->result_sym)
		return -1;
	if (lhs->result_sym > rhs->result_sym)
		return 1;

	return 0;
}

static int compare_compose_candidates_by_sequence(const void *pa, const void *pb)
{
	const struct compose_candidate *lhs = pa;
	const struct compose_candidate *rhs = pb;

	if (lhs->seq[0] < rhs->seq[0])
		return -1;
	if (lhs->seq[0] > rhs->seq[0])
		return 1;
	if (lhs->seq[1] < rhs->seq[1])
		return -1;
	if (lhs->seq[1] > rhs->seq[1])
		return 1;
	if (lhs->score > rhs->score)
		return -1;
	if (lhs->score < rhs->score)
		return 1;
	if (lhs->result_sym < rhs->result_sym)
		return -1;
	if (lhs->result_sym > rhs->result_sym)
		return 1;

	return 0;
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

	if (lhs->diacr < rhs->diacr)
		return -1;
	if (lhs->diacr > rhs->diacr)
		return 1;
	if (lhs->base < rhs->base)
		return -1;
	if (lhs->base > rhs->base)
		return 1;
	if (lhs->result < rhs->result)
		return -1;
	if (lhs->result > rhs->result)
		return 1;

	return 0;
}

static size_t xkeymap_select_compose_candidates(struct compose_candidate *candidates,
						size_t count)
{
	size_t out = 0;

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
	candidate->diacr.diacr = (unsigned int) code;
	candidate->seq[0] = syms[0];

	if (!xkeymap_symbol_is_reachable(xkeymap, syms[1], &code))
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
	int ret = -1;

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

	for (size_t i = 0; i < count; i++) {
		struct lk_kbdiacr diacr;

		rule = malloc(sizeof(*rule));
		if (!rule) {
			ret = -1;
			break;
		}

		*rule = candidates[i].diacr;
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
		XKEYMAP_WARNING(0, "kernel can handle only %d compose definitions, but xkb needs %zu.",
				MAX_DIACR, kernel_rules);
	}
	free(candidates);

	return ret;
}

static int load_translation_table(struct xkeymap *xkeymap)
{
	const char *filename = DATADIR "/xkbtrans/names";
	const char *display_name = filename;
	FILE *fp;

	fp = fopen(filename, "r");
	if (!fp) {
		filename = getenv("LK_XKB_TRANSLATION_TABLE");
		display_name = filename;

		if (filename)
			fp = fopen(filename, "r");

		if (!fp) {
			XKEYMAP_WARNING(0, "%s: translation table not found. Use the LK_XKB_TRANSLATION_TABLE environment variable to specify this file.",
					display_name ? display_name : "<unset>");
			return -1;
		}
	}

	if (parsemap(xkeymap, fp, filename) < 0) {
		fclose(fp);
		XKEYMAP_WARNING(0, "unable to parse xkb translation names");
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

	lk_set_keywords(ctx, LK_KEYWORD_ALTISMETA | LK_KEYWORD_STRASUSUAL);

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
		XKEYMAP_WARNING(0, "too many layouts specified. At the moment, you can use no more than %d", NR_LAYOUTS);
		goto end;
	}

	if (params->locale) {
		xkeymap.compose = xkb_compose_table_new_from_locale(xkeymap.xkb, params->locale, XKB_COMPOSE_COMPILE_NO_FLAGS);
		if (!xkeymap.compose) {
			XKEYMAP_WARNING(0, "xkb compose table for locale `%s' was not found; continuing without compose support",
					params->locale);
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
	if (xkeymap.reachable_syms)
		tdestroy(xkeymap.reachable_syms, free);

	if (xkeymap.syms_map)
		tdestroy(xkeymap.syms_map, xkeymap_pair_free);

	if (xkeymap.compose)
		xkb_compose_table_unref(xkeymap.compose);

	if (xkeymap.keymap)
		xkb_keymap_unref(xkeymap.keymap);
	if (xkeymap.xkb)
		xkb_context_unref(xkeymap.xkb);

	return ret;
}
