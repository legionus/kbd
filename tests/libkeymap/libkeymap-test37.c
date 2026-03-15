#include <stdio.h>
#include <stdlib.h>

#include "libkeymap-test.h"

static const char keymap_text[] = "string a = \"not-a-function\"\n";

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct parsed_keymap keymap;

	if (parse_test_keymap_string(&keymap, "inline-bad-string.map", keymap_text) == 0)
		kbd_error(EXIT_FAILURE, 0, "Unexpected success for non-function string symbol");

	if (lk_func_exists(keymap.ctx, 0))
		kbd_error(EXIT_FAILURE, 0, "Parser stored a function string after rejecting input");

	free_test_keymap(&keymap);

	return EXIT_SUCCESS;
}
