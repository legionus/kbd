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

	expect_key_code(keymap.ctx, 1 << KG_ALT, 16, K(KT_META, 'q'));
	expect_key_code(keymap.ctx, 1 << KG_ALTGR, 16, K_HOLE);

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

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	test_basic_us_layout();
	test_german_altgr("pc104");
	test_german_altgr("pc105");
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
	test_modifier_mask_lookup_across_layouts();

	return EXIT_SUCCESS;
}
