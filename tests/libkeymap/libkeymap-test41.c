#include <stdio.h>
#include <stdlib.h>

#include "libkeymap-test.h"

static const char keymap_text[] =
	"keycode 30 = +a\n"
	"keycode 31 = +0xE9\n";

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct parsed_keymap keymap;
	int value;

	if (parse_test_keymap_string(&keymap, "inline-plus-rvalue.map", keymap_text) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to parse +rvalue entries");

	value = lk_get_key(keymap.ctx, 0, 30);
	if (value != lk_add_capslock(keymap.ctx, K(KT_LATIN, 'a')))
		kbd_error(EXIT_FAILURE, 0, "Unexpected +literal result");

	value = lk_get_key(keymap.ctx, 0, 31);
	if (value != lk_add_capslock(keymap.ctx, 0x00e9))
		kbd_error(EXIT_FAILURE, 0, "Unexpected +number result");

	free_test_keymap(&keymap);

	return EXIT_SUCCESS;
}
