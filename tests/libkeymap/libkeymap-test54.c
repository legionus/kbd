#include <linux/keyboard.h>

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
expect_key_symbol(struct lk_ctx *ctx, int table, int keycode, const char *expected)
{
	int code = lk_get_key(ctx, table, keycode);
	char *actual;

	if (code == K_HOLE)
		kbd_error(EXIT_FAILURE, 0, "Missing keycode %d in table %d", keycode, table);

	if (expected[0] != '\0' && expected[1] == '\0' &&
	    code >= 0 && code < 0x1000 &&
	    (KTYP(code) == KT_LATIN || KTYP(code) == KT_LETTER) &&
	    KVAL(code) == (unsigned char) expected[0])
		return;

	actual = lk_code_to_ksym(ctx, code);
	if (!actual)
		kbd_error(EXIT_FAILURE, 0, "Unable to stringify keycode %d in table %d (raw=0x%x)",
			  keycode, table, code);

	if (strcmp(actual, expected) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unexpected symbol in table %d keycode %d: got %s expected %s",
			  table, keycode, actual, expected);

	free(actual);
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

	expect_key_symbol(keymap.ctx, 0, 16, "q");
	expect_key_symbol(keymap.ctx, 1 << KG_SHIFT, 16, "Q");
	if (KTYP(lk_get_key(keymap.ctx, 1 << KG_SHIFT, 16)) != KT_LATIN)
		kbd_error(EXIT_FAILURE, 0, "Shifted latin level must not be CapsLock-tagged");
	expect_key_symbol(keymap.ctx, 1 << KG_SHIFT, 42, "Shift");

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
	expect_key_symbol(keymap.ctx, 1 << KG_SHIFTL, 58, "ShiftR_Lock");
	expect_key_symbol(keymap.ctx, 0, 16, "q");
	expect_key_symbol(keymap.ctx, 1 << KG_SHIFT, 16, "Q");
	if (KTYP(lk_get_key(keymap.ctx, 1 << KG_SHIFT, 16)) != KT_LATIN)
		kbd_error(EXIT_FAILURE, 0, "Shifted latin level must not be CapsLock-tagged");
	expect_key_symbol(keymap.ctx, 1 << KG_SHIFT, 42, "Shift");
	expect_key_symbol(keymap.ctx, 1 << KG_CTRL, 29, "Control");
	expect_key_symbol(keymap.ctx, 1 << KG_ALT, 56, "Alt");
	expect_key_symbol(keymap.ctx, 1 << KG_SHIFTL, 16, "cyrillic_small_letter_short_i");
	expect_key_symbol(keymap.ctx, (1 << KG_SHIFTL) | (1 << KG_SHIFT), 16,
			  "cyrillic_capital_letter_short_i");

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

	expect_key_symbol(keymap.ctx, 0, 30, "a");
	expect_key_symbol(keymap.ctx, 1 << KG_SHIFT, 30, "A");
	if (KTYP(lk_get_key(keymap.ctx, 1 << KG_SHIFT, 30)) != KT_LATIN)
		kbd_error(EXIT_FAILURE, 0, "Prefer-unicode must not change shifted latin binding type");

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

	expect_key_symbol(keymap.ctx, 0, 16, "q");
	expect_key_symbol(keymap.ctx, 1 << KG_SHIFT, 16, "Q");

	if (lk_get_key(keymap.ctx, 1 << KG_ALT, 16) != K_HOLE)
		kbd_error(EXIT_FAILURE, 0, "LevelFive symbols must not be collapsed into Alt tables");

	free_test_keymap(&keymap);
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	test_basic_us_layout();
	test_group_toggle_layout();
	test_prefer_unicode_does_not_change_xkb_lookup();
	test_level5_is_not_collapsed_into_alt();

	return EXIT_SUCCESS;
}
