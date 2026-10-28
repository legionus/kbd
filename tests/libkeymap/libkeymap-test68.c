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
	if (setenv("XKB_LOG_LEVEL", "critical", 1) != 0)
		kbd_error(EXIT_FAILURE, errno, "unable to set XKB_LOG_LEVEL");
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

static void
expect_options(const char *options, int want_accepted)
{
	struct parsed_keymap keymap;
	struct xkeymap_params params = {
		.model = "pc105",
		.layout = "us,ru",
		.options = options,
	};
	int ret;

	init_test_keymap(&keymap, "xkb-options");
	ret = convert_xkb_keymap(keymap.ctx, &params);
	if ((ret == 0) != want_accepted)
		kbd_error(EXIT_FAILURE, 0, "Unexpected conversion result for options %s: %d",
			  options ? options : "(default)", ret);
	if (!want_accepted) {
		for (unsigned int table = 0; table < MAX_NR_KEYMAPS; table++) {
			if (lk_map_exists(keymap.ctx, table))
				kbd_error(EXIT_FAILURE, 0, "Rejected options populated table %u", table);
		}
	}
	free_test_keymap(&keymap);
}

static void
test_unsupported_options(void)
{
	const char *env = getenv("XKB_DEFAULT_OPTIONS");
	char *saved = env ? strdup(env) : NULL;

	if (env && !saved)
		kbd_error(EXIT_FAILURE, errno, "Unable to save XKB_DEFAULT_OPTIONS");

	expect_options("grp:shifts_toggle", 0);
	expect_options("grp:shifts_toggle,grp:caps_toggle", 0);
	expect_options("grp:caps_toggle,grp:shifts_toggle,grp:switch", 0);
	expect_options("grp:caps_toggle,grp:shifts_toggle", 0);
	expect_options(",, grp:shifts_toggle ,", 0);
	expect_options("grp:shifts_toggle!1", 0);
	expect_options("grp:caps_toggle", 1);
	expect_options("grp:shifts_toggle_extra", 1);
	expect_options("prefix_grp:shifts_toggle", 1);

	if (setenv("XKB_DEFAULT_OPTIONS", "grp:shifts_toggle", 1) != 0)
		kbd_error(EXIT_FAILURE, errno, "Unable to set XKB_DEFAULT_OPTIONS");
	expect_options(NULL, 0);
	expect_options("", 1);
	expect_options("grp:caps_toggle", 1);
	if ((saved ? setenv("XKB_DEFAULT_OPTIONS", saved, 1) : unsetenv("XKB_DEFAULT_OPTIONS")) != 0)
		kbd_error(EXIT_FAILURE, errno, "Unable to restore XKB_DEFAULT_OPTIONS");
	free(saved);
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

	/* This valid variant is listed only in evdev.extras.xml. */
	expect_rmlvo("pc104/us(intl-unicode)", "pc104", "us", "intl-unicode", 1);

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

	test_unsupported_options();

	return EXIT_SUCCESS;
}
