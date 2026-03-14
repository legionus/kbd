#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "libkeymap-test.h"

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	int i;
	char str[] = "qwertyuiopasdfghjklzxcvbnm";
	struct parsed_keymap keymap;

	load_test_keymap(&keymap, "keymap3.map");

	for (i = 0; i < 26; i++) {
		int c = lk_get_key(keymap.ctx, i, 17);
		if (KVAL(c) != str[i])
			kbd_error(EXIT_FAILURE, 0, "Unable to get keycode");
	}

	free_test_keymap(&keymap);

	return EXIT_SUCCESS;
}
