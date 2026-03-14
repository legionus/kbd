#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "libkeymap-test.h"

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	int i = 0;
	struct parsed_keymap keymap;

	load_test_keymap(&keymap, "keymap2.map");

	while (i < MAX_NR_KEYMAPS) {
		int c = lk_get_key(keymap.ctx, i, 17);
		if (KVAL(c) != 'x')
			kbd_error(EXIT_FAILURE, 0, "Unable to get keycode");
		i++;
	}

	free_test_keymap(&keymap);

	return EXIT_SUCCESS;
}
