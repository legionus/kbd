#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libkeymap-test.h"

static const char keymap_text[] = "charset \"definitely-not-a-charset\"\n";

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct parsed_keymap keymap;

	if (parse_test_keymap_string(&keymap, "inline-bad-charset.map", keymap_text) == 0)
		kbd_error(EXIT_FAILURE, 0, "Unexpected success for unknown charset");

	if (strcmp(lk_get_charset(keymap.ctx), "iso-8859-1") != 0)
		kbd_error(EXIT_FAILURE, 0, "Unknown charset changed the default charset");

	if (lk_get_keywords(keymap.ctx) & LK_KEYWORD_CHARSET)
		kbd_error(EXIT_FAILURE, 0, "Unknown charset unexpectedly set the charset keyword");

	free_test_keymap(&keymap);

	return EXIT_SUCCESS;
}
