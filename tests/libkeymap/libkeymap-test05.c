#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "libkeymap-test.h"

static void check_get_sym(struct lk_ctx *ctx, int ktyp, int index, const char *expect)
{
	char *s = lk_get_sym(ctx, ktyp, index);

	if (!s)
		kbd_error(EXIT_FAILURE, 0, "Unable to get key symbol with index %d", index);

	if (strcmp(s, expect))
		kbd_error(EXIT_FAILURE, 0, "Unexpected symbol at index %d", index);

	free(s);
}

static void check_code_to_ksym(struct lk_ctx *ctx, int code, const char *expect)
{
	char *s = lk_code_to_ksym(ctx, code);

	if (!s)
		kbd_error(EXIT_FAILURE, 0, "Unable to convert code (%d) to symbol", code);

	if (strcmp(s, expect))
		kbd_error(EXIT_FAILURE, 0, "Unexpected symbol '%s' for code %d", s, code);

	free(s);
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	int c;
	struct parsed_keymap keymap;

	load_test_keymap(&keymap, "keymap0.map");

	c = lk_get_key(keymap.ctx, 0, 16);
	if (KVAL(c) != 'q')
		kbd_error(EXIT_FAILURE, 0, "Unable to get keycode 16");

	c = lk_get_key(keymap.ctx, 0, 17);
	if (KVAL(c) != 'w')
		kbd_error(EXIT_FAILURE, 0, "Unable to get keycode 17");

	c = lk_get_key(keymap.ctx, 0, 18);
	if (KVAL(c) != 'e')
		kbd_error(EXIT_FAILURE, 0, "Unable to get keycode 18");

	c = lk_get_key(keymap.ctx, 0, 19);
	if (KVAL(c) != 'r')
		kbd_error(EXIT_FAILURE, 0, "Unable to get keycode 19");

	c = lk_get_key(keymap.ctx, 0, 20);
	if (KVAL(c) != 't')
		kbd_error(EXIT_FAILURE, 0, "Unable to get keycode 20");

	c = lk_get_key(keymap.ctx, 0, 21);
	if (KVAL(c) != 'y')
		kbd_error(EXIT_FAILURE, 0, "Unable to get keycode 21");

	if (lk_get_sym(keymap.ctx, 1024, 1) != NULL)
		kbd_error(EXIT_FAILURE, 0, "Unexpected return for unknown ktype");

	if (lk_get_sym(keymap.ctx, KT_LATIN, 512) != NULL)
		kbd_error(EXIT_FAILURE, 0, "Unexpected return for index out of range");

	check_get_sym(keymap.ctx, KT_LATIN, 65, "A");
	check_get_sym(keymap.ctx, KT_LATIN, 66, "B");
	check_get_sym(keymap.ctx, KT_LATIN, 67, "C");

	check_code_to_ksym(keymap.ctx, K(KT_LATIN, 65), "A");
	check_code_to_ksym(keymap.ctx, K(KT_LATIN, 66), "B");
	check_code_to_ksym(keymap.ctx, K(KT_LATIN, 67), "C");

	if (lk_convert_code(keymap.ctx, (int) 'A', TO_AUTO) != 'A')
		kbd_error(EXIT_FAILURE, 0, "Unexpected code after conversion of 'A'");

	free_test_keymap(&keymap);

	return EXIT_SUCCESS;
}
