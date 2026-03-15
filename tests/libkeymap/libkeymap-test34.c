#include <stdio.h>
#include <stdlib.h>

#include "libkeymap-test.h"

static const char keymap_text[] = "keymaps 0-\n";

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct parsed_keymap keymap;

	if (parse_test_keymap_string(&keymap, "inline-parse-error.map", keymap_text) == 0)
		kbd_error(EXIT_FAILURE, 0, "Unexpected success for malformed keymaps line");

	if (lk_get_keywords(keymap.ctx) & LK_KEYWORD_KEYMAPS)
		kbd_error(EXIT_FAILURE, 0, "Malformed keymaps line changed parser state");

	free_test_keymap(&keymap);

	return EXIT_SUCCESS;
}
