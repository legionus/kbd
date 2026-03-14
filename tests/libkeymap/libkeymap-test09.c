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

	setenv("LOADKEYS_INCLUDE_PATH", TESTDIR "/data/libkeymap", 1);

	load_test_keymap(&keymap, "keymap4.map");

	c = lk_get_key(keymap.ctx, 0, 16);
	if (KVAL(c) != 'q')
		kbd_error(EXIT_FAILURE, 0, "Unable to get keycode");

	c = lk_get_key(keymap.ctx, 0, 17);
	if (KVAL(c) != 'w')
		kbd_error(EXIT_FAILURE, 0, "Include40.map failed");

	c = lk_get_key(keymap.ctx, 0, 18);
	if (KVAL(c) != 'e')
		kbd_error(EXIT_FAILURE, 0, "Include41.map failed");

	free_test_keymap(&keymap);

	return EXIT_SUCCESS;
}
