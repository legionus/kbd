#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "libkeymap-test.h"

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	const char *s;
	struct parsed_keymap keymap;

	load_test_keymap(&keymap, "charset-keymap0.map");

	s = lk_get_charset(keymap.ctx);

	if (strcmp(s, "iso-8859-2"))
		kbd_error(EXIT_FAILURE, 0, "Unable to parse charset");

	free_test_keymap(&keymap);

	return EXIT_SUCCESS;
}
