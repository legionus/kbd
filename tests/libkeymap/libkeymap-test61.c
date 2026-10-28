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
set_xcomposefile(const char *name)
{
	char path[512];

	if (snprintf(path, sizeof(path), "%s/data/xkb/compose/%s", TESTDIR, name) >= (int) sizeof(path))
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
test_without_compose_rules(const char *name, int available)
{
	struct parsed_keymap keymap;
	struct xkeymap_params params = {
		.model = "pc104",
		.layout = "us",
		.locale = "en_US.UTF-8",
	};

	init_test_keymap(&keymap, "xkb-us-no-compose");
	set_xkb_config_root();
	set_xcomposefile(name);
	set_xkb_quiet_logging();

	/* The empty-table case must reach the importer, not the missing-file fallback. */
	if (available) {
		struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
		struct xkb_compose_table *table;
		struct xkb_compose_table_iterator *iter;

		if (!context)
			kbd_error(EXIT_FAILURE, 0, "Unable to create XKB context");

		table = xkb_compose_table_new_from_locale(context, params.locale, XKB_COMPOSE_COMPILE_NO_FLAGS);

		if (!table)
			kbd_error(EXIT_FAILURE, 0, "Unable to read empty compose table");

		iter = xkb_compose_table_iterator_new(table);

		if (!iter || xkb_compose_table_iterator_next(iter))
			kbd_error(EXIT_FAILURE, 0, "Expected a compose table with no rules");

		xkb_compose_table_iterator_free(iter);
		xkb_compose_table_unref(table);
		xkb_context_unref(context);
	}

	if (convert_xkb_keymap(keymap.ctx, &params) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to convert XKB us layout with compose file %s", name);

	expect_key_symbol(keymap.ctx, 0, 16, "q");
	expect_key_symbol(keymap.ctx, 1 << KG_SHIFT, 16, "Q");

	if (available && lk_diacr_exists(keymap.ctx, 0))
		kbd_error(EXIT_FAILURE, 0, "Empty compose table unexpectedly added a rule");

	free_test_keymap(&keymap);
}

int main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	test_without_compose_rules("does-not-exist", 0);
	test_without_compose_rules("empty", 1);
	return EXIT_SUCCESS;
}
