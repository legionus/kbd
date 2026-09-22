#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

/*
 * Run convert_xkb_keymap on the given RMLVO and assert that its result
 * matches want_accepted.  The validator runs before any keymap work, so an
 * unrecognized model/layout/variant must cause a non-zero return.
 */
static void
expect_rmlvo(const char *what,
	     const char *model, const char *layout, const char *variant,
	     int want_accepted)
{
	struct parsed_keymap keymap;
	struct xkeymap_params params = {
		.model = model,
		.layout = layout,
		.variant = variant,
	};
	int ret;

	init_test_keymap(&keymap, "xkb-rmlvo");
	set_xkb_config_root();
	set_xkb_suppress_warnings();

	ret = convert_xkb_keymap(keymap.ctx, &params);
	free_test_keymap(&keymap);

	if (want_accepted && ret != 0)
		kbd_error(EXIT_FAILURE, 0, "valid RMLVO was rejected: %s", what);
	if (!want_accepted && ret == 0)
		kbd_error(EXIT_FAILURE, 0, "RMLVO was accepted, but must be rejected: %s", what);
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	/*
	 * Valid combos.  These double as positive controls that guard against
	 * a validator that rejects everything; if that regressed, the tests
	 * below would silently still pass.
	 */
	expect_rmlvo("pc104/us",                    "pc104", "us",    NULL,           1);
	expect_rmlvo("pc104/awesome",               "pc104", "awesome", NULL,         1);
	expect_rmlvo("pc104/us(level5_test)",       "pc104", "us",    "level5_test", 1);

	/*
	 * Unrecognized model.  This used to fall through silently to the
	 * default keycodes because the evdev rules use a wildcard for every
	 * model.
	 */
	expect_rmlvo("pc999_not_a_model/us",        "pc999_not_a_model", "us",  NULL,  0);

	/* Unrecognized layout.  Previously only rejected at the symbols compile step. */
	expect_rmlvo("pc104/not_a_layout_xyz",      "pc104", "not_a_layout_xyz", NULL, 0);

	/* Unrecognized variant.  Previously only rejected at the symbols compile step. */
	expect_rmlvo("pc104/us(not_a_variant_xyz)", "pc104", "us",    "not_a_variant_xyz", 0);

	return EXIT_SUCCESS;
}
