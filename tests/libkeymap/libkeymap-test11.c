#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "libkeymap-test.h"

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct kbsentry kbs;
	struct parsed_keymap keymap;

	load_test_keymap(&keymap, "keymap6.map");

	kbs.kb_func      = 0;
	kbs.kb_string[0] = 0;
	if (lk_get_func(keymap.ctx, &kbs) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to get func 0");

	kbs.kb_func      = 1;
	kbs.kb_string[0] = 0;
	if (lk_get_func(keymap.ctx, &kbs) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to get func 1");

	kbs.kb_func      = 2;
	kbs.kb_string[0] = 0;
	if (lk_get_func(keymap.ctx, &kbs) != -1)
		kbd_error(EXIT_FAILURE, 0, "Possible to get not alloced func");

	free_test_keymap(&keymap);

	return EXIT_SUCCESS;
}
