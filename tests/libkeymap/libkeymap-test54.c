#include <linux/keyboard.h>

#include "libkeymap-test.h"
#include "xkbsupport.h"

static void
set_xkb_translation_table(void)
{
	char path[512];

	if (snprintf(path, sizeof(path), "%s/../data/xkbtrans/names", TESTDIR) >= (int) sizeof(path))
		kbd_error(EXIT_FAILURE, 0, "translation table path is too long");

	if (setenv("LK_XKB_TRANSLATION_TABLE", path, 1) != 0)
		kbd_error(EXIT_FAILURE, errno, "unable to set LK_XKB_TRANSLATION_TABLE");
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
	set_xkb_translation_table();

	if (convert_xkb_keymap(keymap.ctx, &params) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to convert XKB us layout");

	expect_key_symbol(keymap.ctx, 0, 16, "q");
	expect_key_symbol(keymap.ctx, 1 << KG_SHIFT, 16, "Q");

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
	set_xkb_translation_table();

	if (convert_xkb_keymap(keymap.ctx, &params) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to convert XKB us,ru layout");

	expect_key_symbol(keymap.ctx, 0, 58, "ShiftL_Lock");
	expect_key_symbol(keymap.ctx, 1 << KG_SHIFTL, 58, "ShiftR_Lock");
	expect_key_symbol(keymap.ctx, 0, 16, "q");
	expect_key_symbol(keymap.ctx, 1 << KG_SHIFT, 16, "Q");
	expect_key_symbol(keymap.ctx, 1 << KG_SHIFTL, 16, "cyrillic_small_letter_short_i");
	expect_key_symbol(keymap.ctx, (1 << KG_SHIFTL) | (1 << KG_SHIFT), 16,
			  "cyrillic_capital_letter_short_i");

	free_test_keymap(&keymap);
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	test_basic_us_layout();
	test_group_toggle_layout();

	return EXIT_SUCCESS;
}
