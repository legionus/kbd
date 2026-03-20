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
set_missing_xcomposefile(void)
{
	char path[512];

	if (snprintf(path, sizeof(path), "%s/data/xkb/compose/does-not-exist", TESTDIR) >= (int) sizeof(path))
		kbd_error(EXIT_FAILURE, 0, "compose file path is too long");

	if (setenv("XCOMPOSEFILE", path, 1) != 0)
		kbd_error(EXIT_FAILURE, errno, "unable to set XCOMPOSEFILE");
}

static void
set_xkb_quiet_logging(void)
{
	if (setenv("XKB_LOG_LEVEL", "critical", 1) != 0)
		kbd_error(EXIT_FAILURE, errno, "unable to set XKB_LOG_LEVEL");

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

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct parsed_keymap keymap;
	struct xkeymap_params params = {
		.model = "pc104",
		.layout = "us",
		.locale = "en_US.UTF-8",
	};

	init_test_keymap(&keymap, "xkb-us-no-compose");
	set_xkb_config_root();
	set_missing_xcomposefile();
	set_xkb_quiet_logging();

	if (convert_xkb_keymap(keymap.ctx, &params) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to convert XKB us layout without compose table");

	expect_key_symbol(keymap.ctx, 0, 16, "q");
	expect_key_symbol(keymap.ctx, 1 << KG_SHIFT, 16, "Q");

	free_test_keymap(&keymap);
	return EXIT_SUCCESS;
}
