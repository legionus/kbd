#include <linux/keyboard.h>
#include <unistd.h>

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
set_filtered_xkb_translation_table(void)
{
	char src_path[512];
	char tmp_path[] = "/tmp/libkeymap-xkbtrans-XXXXXX";
	FILE *src, *dst;
	char line[512];
	int fd;

	if (snprintf(src_path, sizeof(src_path), "%s/../data/xkbtrans/names", TESTDIR) >= (int) sizeof(src_path))
		kbd_error(EXIT_FAILURE, 0, "translation table path is too long");

	src = fopen(src_path, "r");
	if (!src)
		kbd_error(EXIT_FAILURE, errno, "unable to open %s", src_path);

	fd = mkstemp(tmp_path);
	if (fd < 0)
		kbd_error(EXIT_FAILURE, errno, "unable to create temporary translation table");

	dst = fdopen(fd, "w");
	if (!dst)
		kbd_error(EXIT_FAILURE, errno, "unable to open temporary translation table");

	while (fgets(line, sizeof(line), src)) {
		if (strncmp(line, "Macedonia_", strlen("Macedonia_")) == 0)
			continue;
		if (strncmp(line, "Ukrainian_", strlen("Ukrainian_")) == 0)
			continue;

		if (fputs(line, dst) == EOF)
			kbd_error(EXIT_FAILURE, errno, "unable to write temporary translation table");
	}

	if (fclose(src) != 0)
		kbd_error(EXIT_FAILURE, errno, "unable to close %s", src_path);

	if (fclose(dst) != 0)
		kbd_error(EXIT_FAILURE, errno, "unable to close temporary translation table");

	if (setenv("LK_XKB_TRANSLATION_TABLE", tmp_path, 1) != 0)
		kbd_error(EXIT_FAILURE, errno, "unable to set LK_XKB_TRANSLATION_TABLE");

	if (unlink(tmp_path) != 0)
		kbd_error(EXIT_FAILURE, errno, "unable to unlink temporary translation table");
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
		.model = "pc104",
		.layout = "ru",
		.variant = "chu",
	};

	init_test_keymap(&keymap, "xkb-ru-chu-unicode-fallback");
	set_xkb_config_root();
	set_filtered_xkb_translation_table();
	set_xkb_suppress_warnings();

	if (convert_xkb_keymap(keymap.ctx, &params) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to convert XKB ru(chu) layout");

	expect_key_symbol(keymap.ctx, 0, 41, "ukrainian_cyrillic_small_letter_yi");
	expect_key_symbol(keymap.ctx, 1 << KG_SHIFT, 41, "ukrainian_cyrillic_capital_letter_yi");
	expect_key_symbol(keymap.ctx, 0, 3, "ukrainian_cyrillic_small_letter_ie");
	expect_key_symbol(keymap.ctx, 1 << KG_SHIFT, 3, "ukrainian_cyrillic_capital_letter_ie");
	expect_key_symbol(keymap.ctx, 0, 10, "macedonian_cyrillic_small_letter_dze");
	expect_key_symbol(keymap.ctx, 1 << KG_SHIFT, 10, "macedonian_cyrillic_capital_letter_dze");

	free_test_keymap(&keymap);
	return EXIT_SUCCESS;
}
