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

	actual = lk_code_to_ksym(ctx, code);
	if (!actual)
		kbd_error(EXIT_FAILURE, 0, "Unable to stringify keycode %d in table %d (raw=0x%x)",
			  keycode, table, code);

	if (strcmp(actual, expected) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unexpected symbol in table %d keycode %d: got %s expected %s",
			  table, keycode, actual, expected);

	free(actual);
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct parsed_keymap keymap;
	struct xkeymap_params params = {
		.model = "pc105",
		.layout = "us,ru",
		.options = "grp:caps_toggle,legacyvt:ctrl_alt",
	};

	init_test_keymap(&keymap, "xkb-legacy-ctrl-alt");
	set_xkb_config_root();
	set_xkb_suppress_warnings();

	if (convert_xkb_keymap(keymap.ctx, &params) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to convert XKB us,ru layout with legacy CTRL+ALT VT type");

	expect_key_symbol(keymap.ctx, (1 << KG_CTRL) | (1 << KG_ALT), 59, "Console_1");
	expect_key_symbol(keymap.ctx, (1 << KG_CTRL) | (1 << KG_ALT), 60, "Console_2");
	expect_key_symbol(keymap.ctx, (1 << KG_CTRL) | (1 << KG_ALT), 87, "Console_11");
	expect_key_symbol(keymap.ctx, (1 << KG_CTRL) | (1 << KG_ALT), 88, "Console_12");
	expect_key_symbol(keymap.ctx, (1 << KG_ALT), 99, "Last_Console");

	free_test_keymap(&keymap);
	return EXIT_SUCCESS;
}
