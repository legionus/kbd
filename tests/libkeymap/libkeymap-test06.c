#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "libkeymap-test.h"

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	int c;
	struct parsed_keymap keymap;

	load_test_keymap(&keymap, "keymap1.map");

	c = lk_get_key(keymap.ctx, 0, 16);
	if (KVAL(c) != 'q')
		kbd_error(EXIT_FAILURE, 0, "Unable to get keycode");

	c = lk_get_key(keymap.ctx, 1, 16);
	if (KVAL(c) != 'Q')
		kbd_error(EXIT_FAILURE, 0, "Unable to get keycode");

	free_test_keymap(&keymap);

	return EXIT_SUCCESS;
}
