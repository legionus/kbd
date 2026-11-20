#include <linux/keyboard.h>
#include <xkbcommon/xkbcommon-keysyms.h>

#include "libkeymap-test.h"
#include "xkbsupport.h"

static void
set_xkb_config_root(void)
{
	char path[512];

	if (snprintf(path, sizeof(path), "%s/data/xkb", TESTDIR) >= (int) sizeof(path))
		kbd_error(EXIT_FAILURE, 0, "xkb config root path is too long");

	if (setenv("XKB_CONFIG_ROOT", path, 1) != 0)
		kbd_error(EXIT_FAILURE, errno, "unable to set XKB_CONFIG_ROOT");
}

static void
set_xkb_suppress_warnings(void)
{
	if (setenv("LK_XKB_SUPPRESS_WARNINGS", "1", 1) != 0)
		kbd_error(EXIT_FAILURE, errno, "unable to set LK_XKB_SUPPRESS_WARNINGS");
}

static void
expect_key_symbol(struct lk_ctx *ctx, unsigned int table, unsigned int keycode, const char *expected)
{
	int code = lk_get_key(ctx, table, keycode);
	char *actual;

	if (code == K_HOLE)
		kbd_error(EXIT_FAILURE, 0, "Missing keycode %u in table %u", keycode, table);

	if (expected[0] != '\0' && expected[1] == '\0' &&
	    code >= 0 && code < 0x1000 &&
	    (KTYP(code) == KT_LATIN || KTYP(code) == KT_LETTER) &&
	    KVAL(code) == (unsigned char) expected[0])
		return;

	actual = lk_code_to_ksym(ctx, code);
	if (!actual)
		kbd_error(EXIT_FAILURE, 0, "Unable to stringify keycode %u in table %u (raw=0x%x)",
			  keycode, table, code);

	if (strcmp(actual, expected) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unexpected symbol in table %u keycode %u: got %s expected %s",
			  table, keycode, actual, expected);

	free(actual);
}

static void
expect_key_code(struct lk_ctx *ctx, unsigned int table, unsigned int keycode, int expected)
{
	int actual = lk_get_key(ctx, table, keycode);

	if (actual != expected)
		kbd_error(EXIT_FAILURE, 0, "Unexpected code in table %u keycode %u: got %#x expected %#x",
			  table, keycode, actual, expected);
}

static void
expect_us_capslock(struct lk_ctx *ctx)
{
	/* Match the KT_LETTER table selection in Linux kbd_keycode(). */
	for (unsigned int shift = 0; shift < 2; shift++) {
		for (unsigned int caps = 0; caps < 2; caps++) {
			unsigned int table = shift << KG_SHIFT;
			int letter = lk_get_key(ctx, table, 30);
			int digit = lk_get_key(ctx, table, 2);

			if (caps && KTYP(letter) == KT_LETTER)
				letter = lk_get_key(ctx, table ^ (1U << KG_SHIFT), 30);
			if (caps && KTYP(digit) == KT_LETTER)
				digit = lk_get_key(ctx, table ^ (1U << KG_SHIFT), 2);
			if (KVAL(letter) != ((shift ^ caps) ? 'A' : 'a') ||
			    digit != (shift ? '!' : '1'))
				kbd_error(EXIT_FAILURE, 0, "Incorrect US output with Shift=%u CapsLock=%u",
					  shift, caps);
		}
	}
}

static void
test_basic_us_layout(void)
{
	struct parsed_keymap keymap;
	struct xkeymap_params params = {
		.model = "pc104",
		.layout = "us",
	};

	init_test_keymap(&keymap, "xkb-us");
	set_xkb_config_root();
	set_xkb_suppress_warnings();

	if (convert_xkb_keymap(keymap.ctx, &params) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to convert XKB us layout");

	if (lk_map_exists(keymap.ctx, 1 << KG_CTRLR))
		kbd_error(EXIT_FAILURE, 0, "Unexpected momentary group table without Mode_switch");
	expect_us_capslock(keymap.ctx);
	expect_key_symbol(keymap.ctx, 0, 16, "q");
	expect_key_symbol(keymap.ctx, 1 << KG_SHIFT, 16, "Q");
	if (KTYP(lk_get_key(keymap.ctx, 1 << KG_SHIFT, 16)) != KT_LETTER)
		kbd_error(EXIT_FAILURE, 0, "Shifted letter must be CapsLock-tagged");
	expect_key_symbol(keymap.ctx, 1 << KG_SHIFT, 42, "Shift");
	expect_key_symbol(keymap.ctx, 1 << KG_SHIFT, 28, "Return");
	expect_key_symbol(keymap.ctx, 1 << KG_SHIFT, 57, "space");
	expect_key_symbol(keymap.ctx, 1 << KG_CTRL, 46, "Control_c");
	expect_key_symbol(keymap.ctx, (1 << KG_CTRL) | (1 << KG_SHIFT), 46, "Control_c");
	expect_key_code(keymap.ctx, 1 << KG_ALT, 30, K(KT_META, 'a'));
	expect_key_code(keymap.ctx, (1 << KG_ALT) | (1 << KG_SHIFT), 30, K(KT_META, 'A'));
	expect_key_code(keymap.ctx, (1 << KG_CTRL) | (1 << KG_ALT), 46, K(KT_META, 3));
	for (unsigned int table = 0; table < 16; table++) {
		int space = (table & (1 << KG_CTRL)) ? 0 : ' ';

		if (table & (1 << KG_ALT))
			space = K(KT_META, space);
		expect_key_code(keymap.ctx, table, 57, space);
		expect_key_code(keymap.ctx, table, 28, K_ENTER);
		expect_key_code(keymap.ctx, table, 42, K_SHIFT);
		expect_key_code(keymap.ctx, table, 29, K_CTRL);
	}

	free_test_keymap(&keymap);
}

static void
test_german_altgr(const char *model)
{
	static const struct {
		unsigned int keycode;
		const char *plain;
		const char *shift;
		const char *altgr;
	} keys[] = {
		{ 8,  "seven", "slash",      "braceleft"    },
		{ 9,  "eight", "parenleft",  "bracketleft"  },
		{ 10, "nine",  "parenright", "bracketright" },
		{ 11, "zero",  "equal",      "braceright"   },
		{ 16, "q",     "Q",          "at"           },
		{ 18, "e",     "E",          "euro"         },
		{ 27, "plus",  "asterisk",   "asciitilde"   },
	};
	struct parsed_keymap keymap;
	struct xkeymap_params params = {
		.model = model,
		.layout = "de",
	};
	unsigned int altgr;

	init_test_keymap(&keymap, "xkb-de-altgr");
	set_xkb_config_root();
	set_xkb_suppress_warnings();

	if (convert_xkb_keymap(keymap.ctx, &params) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to convert XKB de layout");

	/* Select the table using the action installed on the right Alt key. */
	expect_key_symbol(keymap.ctx, 0, 100, "AltGr");
	altgr = 1U << KVAL(lk_get_key(keymap.ctx, 0, 100));
	for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
		expect_key_symbol(keymap.ctx, 0, keys[i].keycode, keys[i].plain);
		expect_key_symbol(keymap.ctx, 1 << KG_SHIFT, keys[i].keycode, keys[i].shift);
		expect_key_symbol(keymap.ctx, altgr, keys[i].keycode, keys[i].altgr);
	}

	/* Fourth-level selection and modifier release must use the same tables. */
	expect_key_symbol(keymap.ctx, altgr | (1 << KG_SHIFT), 27, "macron");
	expect_key_symbol(keymap.ctx, altgr, 100, "AltGr");
	expect_key_symbol(keymap.ctx, altgr | (1 << KG_SHIFT), 100, "AltGr");
	expect_key_symbol(keymap.ctx, altgr, 42, "Shift");
	expect_key_symbol(keymap.ctx, altgr | (1 << KG_SHIFT), 42, "Shift");

	/* Unassigned virtual-modifier slots (<LVL3>, <LVL5>..<HYPR>) must be omitted. */
	if (lk_key_exists(keymap.ctx, 0, 84))
		kbd_error(EXIT_FAILURE, 0, "Unassigned keycode 84 (<LVL3>) should not be emitted");
	for (unsigned int fake = 195; fake <= 199; fake++) {
		if (lk_key_exists(keymap.ctx, 0, fake))
			kbd_error(EXIT_FAILURE, 0, "Unassigned keycode %u should not be emitted", fake);
	}
	expect_key_symbol(keymap.ctx, 0, 56, "Alt");
	expect_key_symbol(keymap.ctx, altgr | (1 << KG_CTRL), 16, "nul");
	expect_key_code(keymap.ctx, altgr | (1 << KG_ALT), 16, K(KT_META, '@'));
	expect_key_symbol(keymap.ctx, (1 << KG_CTRL) | (1 << KG_ALT), 59, "Console_1");

	free_test_keymap(&keymap);
}

static void
test_group_toggle_layout(void)
{
	struct parsed_keymap keymap;
	struct xkeymap_params params = {
		.model = "pc104",
		.layout = "us,ru",
		.options = "grp:caps_toggle",
	};

	init_test_keymap(&keymap, "xkb-us-ru");
	set_xkb_config_root();
	set_xkb_suppress_warnings();

	if (convert_xkb_keymap(keymap.ctx, &params) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to convert XKB us,ru layout");

	expect_key_symbol(keymap.ctx, 0, 58, "ShiftL_Lock");
	expect_key_symbol(keymap.ctx, 1 << KG_SHIFT, 58, "Caps_Lock");
	expect_key_symbol(keymap.ctx, (1 << KG_SHIFTL) | (1 << KG_SHIFT), 58, "Caps_Lock");
	expect_key_symbol(keymap.ctx, 1 << KG_SHIFTL, 58, "ShiftR_Lock");
	expect_us_capslock(keymap.ctx);
	expect_key_symbol(keymap.ctx, 0, 16, "q");
	expect_key_symbol(keymap.ctx, 1 << KG_SHIFT, 16, "Q");
	if (KTYP(lk_get_key(keymap.ctx, 1 << KG_SHIFT, 16)) != KT_LETTER)
		kbd_error(EXIT_FAILURE, 0, "Shifted letter must be CapsLock-tagged");
	expect_key_symbol(keymap.ctx, 1 << KG_SHIFT, 42, "Shift");
	expect_key_symbol(keymap.ctx, 1 << KG_CTRL, 29, "Control");
	expect_key_symbol(keymap.ctx, 1 << KG_ALT, 56, "Alt");
	expect_key_symbol(keymap.ctx, (1 << KG_CTRL) | (1 << KG_ALT), 59, "Console_1");
	expect_key_symbol(keymap.ctx, (1 << KG_CTRL) | (1 << KG_ALT), 88, "Console_12");
	expect_key_symbol(keymap.ctx, 0, 99, "Control_backslash");
	expect_key_symbol(keymap.ctx, 1 << KG_ALT, 99, "Last_Console");
	expect_key_symbol(keymap.ctx, (1 << KG_CTRL) | (1 << KG_ALT), 99, "Last_Console");
	expect_key_symbol(keymap.ctx, 1 << KG_SHIFTL, 16, "cyrillic_small_letter_short_i");
	expect_key_symbol(keymap.ctx, (1 << KG_SHIFTL) | (1 << KG_SHIFT), 16,
			  "cyrillic_capital_letter_short_i");
	expect_key_code(keymap.ctx, (1 << KG_SHIFTL) | (1 << KG_CTRL) | (1 << KG_SHIFT),
			28, K_ENTER);
	expect_key_code(keymap.ctx, (1 << KG_SHIFTL) | (1 << KG_SHIFT), 57, ' ');

	free_test_keymap(&keymap);
}

/* The console resolves both press and release using the current table. */
struct console_state {
	unsigned int down[NR_SHIFT];
	unsigned int shift;
	unsigned int lock;
};

static int
console_key(struct lk_ctx *ctx, struct console_state *state, unsigned int key, int pressed)
{
	int code = lk_get_key(ctx, state->shift ^ state->lock, key);
	unsigned int value = KVAL(code);

	if (KTYP(code) == KT_SHIFT) {
		if (pressed)
			state->down[value]++;
		else if (state->down[value])
			state->down[value]--;
		if (state->down[value])
			state->shift |= 1u << value;
		else
			state->shift &= ~(1u << value);
	} else if (KTYP(code) == KT_LOCK && pressed) {
		state->lock ^= 1u << value;
	}
	return code;
}

static void
test_momentary_group_switch(const char *layouts, const char *variants, unsigned int count)
{
	static const unsigned int initial_groups[4][4] = {
		{ 0, 0, 0, 0 },
		{ 0, 1, 1, 0 },
		{ 0, 1, 2, 0 },
		{ 0, 1, 3, 2 }
	};
	static const int sequences[][4] = {
		{ 100, 42,  -100, -42  }, /* Release the switch with Shift held. */
		{ 100, 42,  -42,  -100 }, /* Release Shift before the switch. */
		{ 42,  100, -42,  -100 }, /* Compose; release Shift first. */
		{ 42,  100, -100, -42  }, /* Compose; release RightAlt first. */
		{ 100, 58,  -58,  -100 }, /* Change the locked group while held. */
	};
	struct parsed_keymap keymap;
	struct xkeymap_params params = {
		.model = "pc105",
		.layout = layouts,
		.variant = variants,
		.options = "grp:switch,grp:caps_toggle",
	};
	struct xkb_rule_names names = {
		.rules = "evdev",
		.model = params.model,
		.layout = params.layout,
		.variant = params.variant,
		.options = params.options,
	};
	struct xkb_context *context;
	struct xkb_keymap *reference;

	init_test_keymap(&keymap, "xkb-momentary-group");
	set_xkb_config_root();
	set_xkb_suppress_warnings();
	if (convert_xkb_keymap(keymap.ctx, &params) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to convert momentary group switch");

	context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	if (!context)
		kbd_error(EXIT_FAILURE, 0, "Unable to create XKB context");
	reference = xkb_keymap_new_from_names(context, &names, XKB_KEYMAP_COMPILE_NO_FLAGS);
	if (!reference)
		kbd_error(EXIT_FAILURE, 0, "Unable to compile reference keymap");

	for (unsigned int group = 0; group < 4; group++) {
		unsigned int locked = group << KG_SHIFTL;

		for (unsigned int mods = 0; mods < 16; mods++) {
			expect_key_code(keymap.ctx, locked | mods | (1U << KG_CTRLR), 100, K_CTRLR);
			expect_key_code(keymap.ctx, locked | mods | (1U << KG_CTRLR), 97, K_CTRL);
			expect_key_code(keymap.ctx, locked | mods | (1U << KG_CTRLR), 42, K_SHIFT);
			expect_key_code(keymap.ctx, locked | mods | (1U << KG_CTRLR), 28, K_ENTER);
		}
		for (unsigned int seq = 0; seq < sizeof(sequences) / sizeof(sequences[0]); seq++) {
			struct console_state console = { .lock = locked };
			struct xkb_state *state;

			/*
			 * Exercise locks while Mode_switch is held. AltGr release
			 * across groups is a separate limitation, as is the existing
			 * three-group lock cycle (which is not a next-group cycle).
			 */
			if (seq == 4 && (count == 3 ||
					 lk_get_key(keymap.ctx, locked, 100) != K_CTRLR))
				continue;
			state = xkb_state_new(reference);
			if (!state)
				kbd_error(EXIT_FAILURE, 0, "Unable to create reference state");
			xkb_state_update_mask(state, 0, 0, 0, 0, 0, initial_groups[count - 1][group]);

			for (unsigned int step = 0; step < 4; step++) {
				int event = sequences[seq][step];
				unsigned int key = (unsigned int) (event < 0 ? -event : event);
				int action = console_key(keymap.ctx, &console, key, event > 0);
				int code, expected;

				if (event == 100 &&
				    xkb_state_key_get_one_sym(state, 100 + 8) == XKB_KEY_Mode_switch &&
				    action != K_CTRLR)
					kbd_error(EXIT_FAILURE, 0, "Mode_switch must use CtrlR");
				if (event == 100 &&
				    xkb_state_key_get_one_sym(state, 100 + 8) == XKB_KEY_Multi_key &&
				    action != K_COMPOSE)
					kbd_error(EXIT_FAILURE, 0, "Shift+RightAlt must remain Compose: %s group %u action %#x", layouts, group, action);
				xkb_state_update_key(state, (xkb_keycode_t) key + 8, event > 0 ? XKB_KEY_DOWN : XKB_KEY_UP);
				code = lk_get_key(keymap.ctx, console.shift ^ console.lock, 30);
				if (KTYP(code) == KT_LETTER)
					code = K(KT_LATIN, KVAL(code));
				expected = (int) xkb_state_key_get_utf32(state, 30 + 8);
				if (lk_convert_code(keymap.ctx, code, TO_UNICODE) != expected)
					kbd_error(EXIT_FAILURE, 0,
						  "Group switch mismatch: %s group %u sequence %u step %u: code %#x expected %#x table %#x",
						  layouts, group, seq, step, code, expected, console.shift ^ console.lock);
			}
			if (console.shift)
				kbd_error(EXIT_FAILURE, 0, "Modifier remains held: %s group %u sequence %u mask %#x", layouts, group, seq, console.shift);
			xkb_state_unref(state);
		}
	}
	xkb_keymap_unref(reference);
	xkb_context_unref(context);
	free_test_keymap(&keymap);
}

static void
test_group_select_layout(const char *layouts, const char *variants, unsigned int count, int temporary)
{
	static const unsigned int initial_groups[4][8] = {
		{ 0, 0, 0, 0 },
		{ 0, 1, 1, 0 },
		{ 0, 1, 2, 0, 0, 2, 1, 0 },
		{ 0, 1, 3, 2, 2, 3, 1, 0 },
	};
	static const int events[] = {
		42,
		58,
		-58,
		58,
		-58,
		-42, /* Select group 2 twice. */
		58,
		-58,
		58,
		-58, /* Select group 1 twice. */
		42,
		58,
		-42,
		-58, /* Release CapsLock after Shift. */
		58,
		42,
		-58,
		-42, /* Release CapsLock with Shift held. */
	};
	struct parsed_keymap keymap;
	struct xkeymap_params params = {
		.model = "pc105",
		.layout = layouts,
		.variant = variants,
		.options = temporary ? "grp:shift_caps_switch,grp:switch" : "grp:shift_caps_switch",
	};
	struct xkb_rule_names names = {
		.rules = "evdev",
		.model = params.model,
		.layout = params.layout,
		.variant = params.variant,
		.options = params.options,
	};
	struct xkb_context *context;
	struct xkb_keymap *reference;
	unsigned int states = count > 2 ? 8 : 4;

	init_test_keymap(&keymap, "xkb-group-select");
	set_xkb_config_root();
	set_xkb_suppress_warnings();

	if (convert_xkb_keymap(keymap.ctx, &params) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to convert XKB shift_caps_switch layout");

	if (lk_map_exists(keymap.ctx, 1 << KG_CTRLL) != (count > 2) ||
	    lk_map_exists(keymap.ctx, 1 << KG_CTRLR) != temporary)
		kbd_error(EXIT_FAILURE, 0, "Unexpected group table allocation");

	for (unsigned int table = 0; table < MAX_NR_KEYMAPS; table++) {
		if (!lk_map_exists(keymap.ctx, table))
			continue;

		expect_key_code(keymap.ctx, table, 29, K_CTRL);
		expect_key_code(keymap.ctx, table, 97, K_CTRL);
		expect_key_code(keymap.ctx, table, 42, K_SHIFT);
		expect_key_code(keymap.ctx, table, 28, K_ENTER);
	}

	context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	if (!context)
		kbd_error(EXIT_FAILURE, 0, "Unable to create XKB context");

	reference = xkb_keymap_new_from_names(context, &names, XKB_KEYMAP_COMPILE_NO_FLAGS);
	if (!reference)
		kbd_error(EXIT_FAILURE, 0, "Unable to compile reference keymap");

	for (unsigned int initial = 0; initial < states; initial++) {
		for (int held = 0; held <= temporary; held++) {
			struct console_state console = { .lock = initial << KG_SHIFTL };
			struct xkb_state *state = xkb_state_new(reference);

			if (!state)
				kbd_error(EXIT_FAILURE, 0, "Unable to create reference state");

			xkb_state_update_mask(state, 0, 0, 0, 0, 0, initial_groups[count - 1][initial]);

			if (held) {
				console_key(keymap.ctx, &console, 100, 1);
				xkb_state_update_key(state, 100 + 8, XKB_KEY_DOWN);
			}

			for (size_t i = 0; i < sizeof(events) / sizeof(events[0]); i++) {
				int event = events[i];
				unsigned int key = (unsigned int) (event < 0 ? -event : event);
				int code, expected;

				console_key(keymap.ctx, &console, key, event > 0);
				xkb_state_update_key(state, (xkb_keycode_t) key + 8,
						     event > 0 ? XKB_KEY_DOWN : XKB_KEY_UP);

				code = lk_get_key(keymap.ctx, console.shift ^ console.lock, 16);

				if (KTYP(code) == KT_LETTER)
					code = K(KT_LATIN, KVAL(code));

				expected = (int) xkb_state_key_get_utf32(state, 16 + 8);

				if (lk_convert_code(keymap.ctx, code, TO_UNICODE) != expected ||
				    (console.lock & ((1 << KG_SHIFT) | (1 << KG_CTRL))))
					kbd_error(EXIT_FAILURE, 0,
						  "Absolute group mismatch: %s state %u held %d event %zu",
						  layouts, initial, held, i);
			}

			if (held) {
				console_key(keymap.ctx, &console, 100, 0);
				xkb_state_update_key(state, 100 + 8, XKB_KEY_UP);
			}

			if (console.shift)
				kbd_error(EXIT_FAILURE, 0, "Modifier stuck after absolute group selection");

			expect_key_symbol(keymap.ctx, console.lock, 16, "q");
			xkb_state_unref(state);
		}
	}
	xkb_keymap_unref(reference);
	xkb_context_unref(context);
	free_test_keymap(&keymap);
}

static void
test_previous_group(const char *layouts, unsigned int count, int temporary)
{
	static const unsigned int initial_groups[4][8] = {
		{ 0, 0, 0, 0 },
		{ 0, 1, 1, 0 },
		{ 0, 1, 2, 0, 0, 2, 1, 0 },
		{ 0, 1, 3, 2, 2, 3, 1, 0 },
	};
	struct parsed_keymap keymap;
	struct xkeymap_params params = {
		.model = "pc105",
		.layout = layouts,
		.options = temporary ? "local:previous_group,grp:switch" : "local:previous_group",
	};
	struct xkb_rule_names names = {
		.rules = "evdev",
		.model = params.model,
		.layout = params.layout,
		.options = params.options,
	};
	struct xkb_context *context;
	struct xkb_keymap *reference;
	unsigned int states = count > 2 ? 8 : 4;

	init_test_keymap(&keymap, "xkb-previous-group");
	set_xkb_config_root();
	set_xkb_suppress_warnings();
	if (convert_xkb_keymap(keymap.ctx, &params) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to convert previous-group keymap");

	if (lk_map_exists(keymap.ctx, 1 << KG_CTRLL) != (count > 2) ||
	    lk_map_exists(keymap.ctx, 1 << KG_CTRLR) != temporary)
		kbd_error(EXIT_FAILURE, 0, "Unexpected previous-group table allocation");

	for (unsigned int table = 0; table < MAX_NR_KEYMAPS; table++) {
		if (!lk_map_exists(keymap.ctx, table))
			continue;
		expect_key_code(keymap.ctx, table, 58, lk_get_key(keymap.ctx, table & ~15U, 58));
		expect_key_code(keymap.ctx, table, 29, K_CTRL);
		expect_key_code(keymap.ctx, table, 97, K_CTRL);
		expect_key_code(keymap.ctx, table, 42, K_SHIFT);
		expect_key_code(keymap.ctx, table, 28, K_ENTER);
	}

	context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	if (!context)
		kbd_error(EXIT_FAILURE, 0, "Unable to create XKB context");
	reference = xkb_keymap_new_from_names(context, &names, XKB_KEYMAP_COMPILE_NO_FLAGS);
	if (!reference)
		kbd_error(EXIT_FAILURE, 0, "Unable to compile previous-group reference");

	for (unsigned int initial = 0; initial < states; initial++) {
		for (int held = 0; held <= temporary; held++) {
			struct console_state console = { .lock = initial << KG_SHIFTL };
			struct xkb_state *state = xkb_state_new(reference);

			if (!state)
				kbd_error(EXIT_FAILURE, 0, "Unable to create reference state");
			xkb_state_update_mask(state, 0, 0, 0, 0, 0, initial_groups[count - 1][initial]);
			if (held) {
				console_key(keymap.ctx, &console, 100, 1);
				xkb_state_update_key(state, 108, XKB_KEY_DOWN);
			}

			/* Two full backward cycles, comparing both press and release. */
			for (unsigned int event = 0; event < 4 * count; event++) {
				int pressed = !(event & 1);
				int action = console_key(keymap.ctx, &console, 58, pressed);
				int code;

				xkb_state_update_key(state, 66, pressed ? XKB_KEY_DOWN : XKB_KEY_UP);
				if (count == 1 && action != K_HOLE)
					kbd_error(EXIT_FAILURE, 0, "Previous group must be a no-op for one layout");
				if (console.lock & ((1 << KG_SHIFT) | (1 << KG_CTRL)))
					kbd_error(EXIT_FAILURE, 0, "Previous group locked an ordinary modifier");

				/* Y differs in all four layouts: y, Cyrillic en, z, upsilon. */
				code = lk_get_key(keymap.ctx, console.shift ^ console.lock, 21);
				if (KTYP(code) == KT_LETTER)
					code = K(KT_LATIN, KVAL(code));
				if (lk_convert_code(keymap.ctx, code, TO_UNICODE) !=
				    (int) xkb_state_key_get_utf32(state, 29))
					kbd_error(EXIT_FAILURE, 0,
						  "Previous-group mismatch: %s initial %u held %d event %u",
						  layouts, initial, held, event);
			}

			if (held) {
				int code;

				console_key(keymap.ctx, &console, 100, 0);
				xkb_state_update_key(state, 108, XKB_KEY_UP);
				code = lk_get_key(keymap.ctx, console.shift ^ console.lock, 21);
				if (KTYP(code) == KT_LETTER)
					code = K(KT_LATIN, KVAL(code));
				if (lk_convert_code(keymap.ctx, code, TO_UNICODE) !=
				    (int) xkb_state_key_get_utf32(state, 29))
					kbd_error(EXIT_FAILURE, 0, "Previous group lost after switch release");
			}
			if (console.shift)
				kbd_error(EXIT_FAILURE, 0, "Modifier stuck after previous-group selection");
			xkb_state_unref(state);
		}
	}
	xkb_keymap_unref(reference);
	xkb_context_unref(context);
	free_test_keymap(&keymap);
}

static void
test_prefer_unicode_does_not_change_xkb_lookup(void)
{
	struct parsed_keymap keymap;
	struct xkeymap_params params = {
		.model = "pc104",
		.layout = "us,ru",
		.options = "grp:caps_toggle",
	};

	init_test_keymap(&keymap, "xkb-us-ru-prefer-unicode");
	set_xkb_config_root();
	set_xkb_suppress_warnings();

	if (lk_set_parser_flags(keymap.ctx, LK_FLAG_PREFER_UNICODE) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to enable prefer-unicode mode");

	if (convert_xkb_keymap(keymap.ctx, &params) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to convert XKB us,ru layout with prefer-unicode");

	expect_us_capslock(keymap.ctx);
	expect_key_symbol(keymap.ctx, 0, 30, "a");
	expect_key_symbol(keymap.ctx, 1 << KG_SHIFT, 30, "A");
	if (KTYP(lk_get_key(keymap.ctx, 1 << KG_SHIFT, 30)) != KT_LETTER)
		kbd_error(EXIT_FAILURE, 0, "Unicode shifted letter must be CapsLock-tagged");

	free_test_keymap(&keymap);
}

static void
test_level5_is_not_collapsed_into_alt(void)
{
	struct parsed_keymap keymap;
	struct xkeymap_params params = {
		.model = "pc104",
		.layout = "us",
		.variant = "level5_test",
	};

	init_test_keymap(&keymap, "xkb-us-level5");
	set_xkb_config_root();
	set_xkb_suppress_warnings();

	if (convert_xkb_keymap(keymap.ctx, &params) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to convert XKB us level5 test layout");

	expect_us_capslock(keymap.ctx);
	expect_key_symbol(keymap.ctx, 0, 16, "q");
	expect_key_symbol(keymap.ctx, 1 << KG_SHIFT, 16, "Q");
	expect_key_code(keymap.ctx, 0, 100, K_CTRLL);
	expect_key_code(keymap.ctx, 1 << KG_CTRLL, 16, K_HOLE);
	expect_key_symbol(keymap.ctx, (1 << KG_CTRLL) | (1 << KG_SHIFT), 16, "exclam");
	expect_key_code(keymap.ctx, (1 << KG_CTRLL) | (1 << KG_ALTGR), 16, K_HOLE);
	expect_key_symbol(keymap.ctx, (1 << KG_CTRLL) | (1 << KG_ALTGR) | (1 << KG_SHIFT), 16, "at");

	expect_key_code(keymap.ctx, 1 << KG_ALT, 16, K(KT_META, 'q'));
	expect_key_code(keymap.ctx, 1 << KG_ALTGR, 16, K_HOLE);

	free_test_keymap(&keymap);
}

static void
test_level5_layouts_and_table_limits(void)
{
	struct parsed_keymap keymap;
	struct xkeymap_params noted = {
		.model = "pc105",
		.layout = "de",
		.variant = "noted",
	};
	struct xkeymap_params e1 = {
		.model = "pc105",
		.layout = "de",
		.variant = "e1",
	};
	struct xkeymap_params t3 = {
		.model = "pc105",
		.layout = "de",
		.variant = "T3",
	};
	struct xkeymap_params digits_lock = {
		.model = "pc105",
		.layout = "fr",
		.options = "caps:digits_row_independent_lock",
	};
	struct xkeymap_params three_group_select = {
		.model = "pc105",
		.layout = "us,ru,de",
		.variant = ",,noted",
		.options = "grp:shift_caps_switch",
	};
	struct xkeymap_params exhausted = {
		.model = "pc105",
		.layout = "us,ru,de",
		.variant = ",,noted",
		.options = "grp:shift_caps_switch,grp:switch",
	};

	init_test_keymap(&keymap, "xkb-de-noted-level5");
	set_xkb_config_root();
	set_xkb_suppress_warnings();

	if (convert_xkb_keymap(keymap.ctx, &noted) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to convert XKB de(noted) layout");

	/* Level3 stays on AltGr; Level5 uses CtrlL and CtrlL_Lock. */
	expect_key_code(keymap.ctx, 0, 58, K_ALTGR);
	expect_key_code(keymap.ctx, 0, 40, K_ALTGR);
	expect_key_code(keymap.ctx, 0, 86, K_CTRLL);
	expect_key_code(keymap.ctx, 0, 100, K_CTRLL);
	expect_key_code(keymap.ctx, (1 << KG_ALTGR) | (1 << KG_SHIFT), 15, K_CTRLLLOCK);
	expect_key_symbol(keymap.ctx, 0, 32, "i");
	expect_key_symbol(keymap.ctx, 1 << KG_SHIFT, 32, "I");
	expect_key_symbol(keymap.ctx, 1 << KG_ALTGR, 32, "braceleft");
	expect_key_symbol(keymap.ctx, (1 << KG_ALTGR) | (1 << KG_SHIFT), 32, "iota");
	expect_key_symbol(keymap.ctx, 1 << KG_CTRLL, 32, "Down");
	expect_key_symbol(keymap.ctx, (1 << KG_CTRLL) | (1 << KG_SHIFT), 32, "Down");
	expect_key_code(keymap.ctx, (1 << KG_CTRLL) | (1 << KG_ALTGR), 32, 0x222b ^ 0xf000);
	free_test_keymap(&keymap);

	/*
	 * Layouts such as de(e1) and de(T3) expose ISO_Level5_Latch only on
	 * higher levels (e.g. AltGr+F or Shift+RightAlt) and rely on the
	 * virtual <LVL5> mapping for its modifier mask. Verify that holding
	 * the level-5 modifier reaches level 5 and releasing keys in any
	 * order never leaves a modifier stuck.
	 */
	init_test_keymap(&keymap, "xkb-de-e1-level5-latch");
	if (convert_xkb_keymap(keymap.ctx, &e1) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to convert XKB de(e1) layout");
	expect_key_code(keymap.ctx, 1 << KG_ALTGR, 33, K_CTRLL);
	expect_key_symbol(keymap.ctx, 1 << KG_CTRLL, 16, "masculine");
	{
		struct console_state console = { 0 };

		console_key(keymap.ctx, &console, 100, 1); /* Press AltGr */
		console_key(keymap.ctx, &console, 33, 1);  /* Press F -> CtrlL */
		console_key(keymap.ctx, &console, 100, 0); /* Release AltGr first */
		if (console_key(keymap.ctx, &console, 16, 1) != K(KT_LETTER, 0xba))
			kbd_error(EXIT_FAILURE, 0, "Held LevelFive did not select level 5 symbol");
		console_key(keymap.ctx, &console, 16, 0);
		console_key(keymap.ctx, &console, 33, 0);  /* Release F */
		if (console.shift != 0)
			kbd_error(EXIT_FAILURE, 0, "Modifier stuck after releasing AltGr before F in de(e1)");
	}
	free_test_keymap(&keymap);

	init_test_keymap(&keymap, "xkb-de-t3-level5-latch");
	if (convert_xkb_keymap(keymap.ctx, &t3) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to convert XKB de(T3) layout");
	expect_key_code(keymap.ctx, 1 << KG_SHIFT, 100, K_ALTGR);
	expect_key_code(keymap.ctx, 1 << KG_ALTGR, 42, K_SHIFT);
	{
		struct console_state console = { 0 };

		console_key(keymap.ctx, &console, 42, 1);  /* Press Shift */
		console_key(keymap.ctx, &console, 100, 1); /* Press RightAlt */
		console_key(keymap.ctx, &console, 42, 0);  /* Release Shift first */
		console_key(keymap.ctx, &console, 100, 0); /* Release RightAlt */
		if (console.shift != 0)
			kbd_error(EXIT_FAILURE, 0, "Modifier stuck after Shift+RightAlt in de(T3)");
	}
	free_test_keymap(&keymap);

	/*
	 * caps:digits_row_independent_lock exposes ISO_Level5_Lock only on
	 * level 2 of CapsLock; modifier mask probing must still fall back to
	 * the virtual <LVL5> keycode to resolve LevelFive.
	 */
	init_test_keymap(&keymap, "xkb-fr-digits-level5-lock");
	if (convert_xkb_keymap(keymap.ctx, &digits_lock) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to convert fr with caps:digits_row_independent_lock");
	expect_key_code(keymap.ctx, 1 << KG_SHIFT, 58, K_CTRLLLOCK);
	expect_key_symbol(keymap.ctx, 0, 2, "ampersand");
	expect_key_symbol(keymap.ctx, 1 << KG_CTRLL, 2, "one");
	free_test_keymap(&keymap);

	/*
	 * When >2 groups with absolute selection reserve KG_CTRLL, LevelFive
	 * falls back to KG_CTRLR as long as momentary group switching is absent.
	 */
	init_test_keymap(&keymap, "xkb-level5-ctrlr-fallback");
	if (convert_xkb_keymap(keymap.ctx, &three_group_select) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to convert 3-group LevelFive layout with group selection");
	expect_key_code(keymap.ctx, 1 << KG_SHIFTR, 86, K_CTRLR);
	expect_key_code(keymap.ctx, 1 << KG_SHIFTR, 100, K_CTRLR);
	expect_key_code(keymap.ctx, (1 << KG_SHIFTR) | (1 << KG_ALTGR) | (1 << KG_SHIFT), 15, K_CTRLRLOCK);
	expect_key_symbol(keymap.ctx, (1 << KG_SHIFTR) | (1 << KG_CTRLR), 32, "Down");
	free_test_keymap(&keymap);

	/*
	 * Combining >2 groups, absolute/previous group selection, momentary
	 * group switching, and LevelFive requires 9 bits (512 tables) and must
	 * be rejected cleanly.
	 */
	init_test_keymap(&keymap, "xkb-level5-exhausted");
	if (convert_xkb_keymap(keymap.ctx, &exhausted) == 0)
		kbd_error(EXIT_FAILURE, 0, "Unexpectedly converted keymap exceeding 256 kernel tables");
	for (unsigned int table = 0; table < MAX_NR_KEYMAPS; table++) {
		if (lk_map_exists(keymap.ctx, table))
			kbd_error(EXIT_FAILURE, 0, "Rejected keymap populated table %u", table);
	}
	free_test_keymap(&keymap);
}

static void
test_modifier_mask_lookup_across_layouts(void)
{
	struct parsed_keymap keymap;
	struct xkeymap_params params = {
		.model = "pc104",
		.layout = "us,local",
		.variant = ",group2_level3_probe",
		.options = "grp:caps_toggle",
	};

	init_test_keymap(&keymap, "xkb-layout-level3-mask");
	set_xkb_config_root();
	set_xkb_suppress_warnings();

	if (convert_xkb_keymap(keymap.ctx, &params) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to convert XKB layout with group2-only LevelThree modifier");

	expect_key_symbol(keymap.ctx, 1 << KG_SHIFTL, 2, "1");
	expect_key_symbol(keymap.ctx, (1 << KG_SHIFTL) | (1 << KG_ALTGR), 2, "exclamdown");
	expect_key_symbol(keymap.ctx, (1 << KG_SHIFTL) | (1 << KG_ALTGR) | (1 << KG_SHIFT), 2,
			  "onesuperior");

	free_test_keymap(&keymap);
}

static void
test_shift_both_capslock_does_not_stick(void)
{
	struct parsed_keymap keymap;
	struct xkeymap_params params = {
		.model = "pc105",
		.layout = "de",
		.variant = "noted",
	};
	struct console_state console = { 0 };

	init_test_keymap(&keymap, "xkb-de-noted-shift");
	set_xkb_config_root();
	set_xkb_suppress_warnings();

	if (convert_xkb_keymap(keymap.ctx, &params) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to convert XKB de(noted) layout");

	for (unsigned int table = 0; table < 64; table++) {
		if (!lk_map_exists(keymap.ctx, table))
			continue;
		expect_key_code(keymap.ctx, table, 42, K_SHIFT);
		expect_key_code(keymap.ctx, table, 54, K_SHIFT);
	}

	console_key(keymap.ctx, &console, 42, 1);
	if (console.shift != (1U << KG_SHIFT))
		kbd_error(EXIT_FAILURE, 0, "Left Shift press did not activate KG_SHIFT");
	console_key(keymap.ctx, &console, 42, 0);
	if (console.shift != 0 || console.lock != 0)
		kbd_error(EXIT_FAILURE, 0, "Left Shift release left modifier stuck");

	console_key(keymap.ctx, &console, 54, 1);
	if (console.shift != (1U << KG_SHIFT))
		kbd_error(EXIT_FAILURE, 0, "Right Shift press did not activate KG_SHIFT");
	console_key(keymap.ctx, &console, 54, 0);
	if (console.shift != 0 || console.lock != 0)
		kbd_error(EXIT_FAILURE, 0, "Right Shift release left modifier stuck");

	free_test_keymap(&keymap);
}

static void
test_real_key_modifier_mask_priority(void)
{
	struct parsed_keymap keymap;
	struct xkeymap_params params = {
		.model = "pc105",
		.layout = "local",
		.variant = "real_key_mod_override",
	};

	init_test_keymap(&keymap, "xkb-real-key-mod-priority");
	set_xkb_config_root();
	set_xkb_suppress_warnings();

	if (convert_xkb_keymap(keymap.ctx, &params) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to convert local(real_key_mod_override)");

	/*
	 * Real keys bind ISO_Level3_Latch -> Mod3 and ISO_Level5_Lock -> Mod5,
	 * overriding the fake <LVL3> (Mod5) and <LVL5> (Mod3) keys in pc(pc105).
	 */
	expect_key_code(keymap.ctx, 0, 100, K_ALTGR);
	expect_key_code(keymap.ctx, 0, 86, K_CTRLLLOCK);
	expect_key_symbol(keymap.ctx, 1 << KG_ALTGR, 2, "section");
	expect_key_symbol(keymap.ctx, 1 << KG_CTRLL, 2, "exclamdown");

	free_test_keymap(&keymap);
}

static void
test_keypad_numlock_semantics(void)
{
	struct parsed_keymap keymap;
	struct xkeymap_params de_pc = {
		.model = "pc105",
		.layout = "de",
	};
	struct xkeymap_params us_mac_numpad = {
		.model = "pc105",
		.layout = "us",
		.options = "numpad:mac",
	};
	struct xkeymap_params de_noted = {
		.model = "pc105",
		.layout = "de",
		.variant = "noted",
	};

	set_xkb_config_root();
	set_xkb_suppress_warnings();

	/* Standard KEYPAD keys paired under NumLock keep KT_PAD; unpaired KP_Decimal does not. */
	init_test_keymap(&keymap, "xkb-keypad-pc");
	if (convert_xkb_keymap(keymap.ctx, &de_pc) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to convert de pc105 layout");
	expect_key_code(keymap.ctx, 0, 71, K_P7);
	expect_key_code(keymap.ctx, 1 << KG_SHIFT, 71, K_P7);
	expect_key_code(keymap.ctx, 0, 83, K_PDOT);
	expect_key_code(keymap.ctx, 0, 121, K(KT_LATIN, '.'));
	free_test_keymap(&keymap);

	/* numpad:mac emits unconditional KP digits/decimal even with NumLock=0. */
	init_test_keymap(&keymap, "xkb-keypad-mac");
	if (convert_xkb_keymap(keymap.ctx, &us_mac_numpad) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to convert us with numpad:mac");
	expect_key_code(keymap.ctx, 0, 71, K(KT_LATIN, '7'));
	expect_key_code(keymap.ctx, 0, 82, K(KT_LATIN, '0'));
	expect_key_code(keymap.ctx, 0, 83, K(KT_LATIN, '.'));
	free_test_keymap(&keymap);

	/* de(noted) has no NumLock: Layer 4 KP digits and KP navigation must be unconditional. */
	init_test_keymap(&keymap, "xkb-keypad-noted");
	if (convert_xkb_keymap(keymap.ctx, &de_noted) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to convert de(noted) layout");
	expect_key_code(keymap.ctx, 1 << KG_CTRLL, 22, K(KT_LATIN, '7'));
	expect_key_code(keymap.ctx, 1 << KG_CTRLL, 57, K(KT_LATIN, '0'));
	expect_key_code(keymap.ctx, (1 << KG_CTRLL) | (1 << KG_SHIFT), 39, K(KT_LATIN, '.'));
	expect_key_code(keymap.ctx, 0, 71, K(KT_LATIN, '7'));
	expect_key_code(keymap.ctx, 1 << KG_CTRLL, 71, K_FIND);
	free_test_keymap(&keymap);
}

static void
test_capslock_follows_xkb_lock_state(void)
{
	struct parsed_keymap keymap;
	struct xkeymap_params params = {
		.model = "pc105",
		.layout = "cz",
	};

	init_test_keymap(&keymap, "xkb-cz-capslock-levels");
	set_xkb_config_root();
	set_xkb_suppress_warnings();

	if (convert_xkb_keymap(keymap.ctx, &params) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to convert XKB cz layout");

	/* Levels 1-2 invert under CapsLock, but Shift+AltGr+E must not flip to EuroSign. */
	expect_key_code(keymap.ctx, 0, 18, K(KT_LETTER, 'e'));
	expect_key_code(keymap.ctx, 1 << KG_SHIFT, 18, K(KT_LETTER, 'E'));
	expect_key_code(keymap.ctx, (1 << KG_SHIFT) | (1 << KG_ALTGR), 18, K(KT_LATIN, 'E'));

	free_test_keymap(&keymap);
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	test_basic_us_layout();
	test_german_altgr("pc104");
	test_german_altgr("pc105");
	test_capslock_follows_xkb_lock_state();
	test_shift_both_capslock_does_not_stick();
	test_group_toggle_layout();
	test_momentary_group_switch("us", NULL, 1);
	test_momentary_group_switch("us,ru", NULL, 2);
	test_momentary_group_switch("us,ru,us", ",,dvorak", 3);
	test_momentary_group_switch("us,ru,us,ru", ",,dvorak,phonetic", 4);
	test_momentary_group_switch("us,ru,de,gr", NULL, 4);
	test_group_select_layout("us", NULL, 1, 1);
	test_group_select_layout("us,ru", NULL, 2, 1);
	test_group_select_layout("us,ru,us", ",,dvorak", 3, 1);
	test_group_select_layout("us,ru,us,ru", ",,dvorak,phonetic", 4, 1);
	test_group_select_layout("us,ru,us,ru", ",,dvorak,phonetic", 4, 0);
	test_previous_group("us", 1, 0);
	test_previous_group("us,ru", 2, 0);
	test_previous_group("us,ru,de", 3, 0);
	test_previous_group("us,ru,de,gr", 4, 0);
	test_previous_group("us", 1, 1);
	test_previous_group("us,ru", 2, 1);
	test_previous_group("us,ru,de", 3, 1);
	test_previous_group("us,ru,de,gr", 4, 1);
	test_prefer_unicode_does_not_change_xkb_lookup();
	test_level5_is_not_collapsed_into_alt();
	test_level5_layouts_and_table_limits();
	test_modifier_mask_lookup_across_layouts();
	test_real_key_modifier_mask_priority();
	test_keypad_numlock_semantics();

	return EXIT_SUCCESS;
}
