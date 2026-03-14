#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "libkeymap-test.h"

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	unsigned int i;
	struct kbsentry kbs;
	struct parsed_keymap keymap;

	load_test_keymap(&keymap, "keymap5.map");

	for (i = 0; i < MAX_NR_FUNC; i++) {
		if (!lk_func_exists(keymap.ctx, (int) i))
			kbd_error(EXIT_FAILURE, 0, "Unable to find func %d", i);

		kbs.kb_func      = (unsigned char) i;
		kbs.kb_string[0] = 0;

		if (lk_get_func(keymap.ctx, &kbs) != 0)
			kbd_error(EXIT_FAILURE, 0, "Unable to get func %d", i);
	}

	if (lk_del_func(keymap.ctx, 1) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to remove func 1");

	if (lk_func_exists(keymap.ctx, 1))
		kbd_error(EXIT_FAILURE, 0, "The func 1 still exist");

	free_test_keymap(&keymap);

	return EXIT_SUCCESS;
}
