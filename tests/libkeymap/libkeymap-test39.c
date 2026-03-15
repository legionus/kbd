#include <stdio.h>
#include <stdlib.h>

#include "libkeymap-test.h"

static const char keymap_text[] = "compose as usual for \"koi8-r\"\n";

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct parsed_keymap keymap;

	if (parse_test_keymap_string(&keymap, "inline-bad-compose.map", keymap_text) == 0)
		kbd_error(EXIT_FAILURE, 0, "Unexpected success for unsupported compose charset");

	if (lk_diacr_exists(keymap.ctx, 0))
		kbd_error(EXIT_FAILURE, 0, "Parser stored compose rules after rejecting charset");

	free_test_keymap(&keymap);

	return EXIT_SUCCESS;
}
