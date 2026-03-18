#include <stdio.h>
#include <stdlib.h>

#include <keymap.h>

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
set_xkb_config_root(void)
{
	char path[512];

	if (snprintf(path, sizeof(path), "%s/data/xkb", TESTDIR) >= (int) sizeof(path))
		kbd_error(EXIT_FAILURE, 0, "xkb config root path is too long");

	if (setenv("XKB_CONFIG_ROOT", path, 1) != 0)
		kbd_error(EXIT_FAILURE, errno, "unable to set XKB_CONFIG_ROOT");
}

static void
set_xcomposefile(void)
{
	char path[512];

	if (snprintf(path, sizeof(path), "%s/data/xkb/compose/en_US", TESTDIR) >= (int) sizeof(path))
		kbd_error(EXIT_FAILURE, 0, "compose file path is too long");

	if (setenv("XCOMPOSEFILE", path, 1) != 0)
		kbd_error(EXIT_FAILURE, errno, "unable to set XCOMPOSEFILE");
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct parsed_keymap keymap;
	struct xkeymap_params params = {
		.model = "pc104",
		.layout = "us,ru",
		.options = "grp:caps_toggle",
		.locale = "en_US.UTF-8",
	};

	init_test_keymap(&keymap, "xkb-us-ru-dump");
	set_xkb_config_root();
	set_xcomposefile();
	set_xkb_translation_table();

	if (convert_xkb_keymap(keymap.ctx, &params) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to convert XKB us,ru layout");

	lk_dump_keymap(keymap.ctx, stdout, LK_SHAPE_SEPARATE_LINES, 0);
	lk_dump_diacs(keymap.ctx, stdout);

	free_test_keymap(&keymap);
	return EXIT_SUCCESS;
}
