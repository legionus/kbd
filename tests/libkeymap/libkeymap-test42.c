#include <stdio.h>
#include <stdlib.h>

#include "libkeymap-test.h"

static const char keymap_text[] =
	"keymaps 0-1\n"
	"keycode 30 = a b c\n";

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct parsed_keymap keymap;

	if (parse_test_keymap_string(&keymap, "inline-too-many-rvalues.map", keymap_text) == 0)
		kbd_error(EXIT_FAILURE, 0, "Unexpected success for too many rvalues");

	if (!(lk_get_keywords(keymap.ctx) & LK_KEYWORD_KEYMAPS))
		kbd_error(EXIT_FAILURE, 0, "The parser lost keymaps state before reporting the error");

	free_test_keymap(&keymap);

	return EXIT_SUCCESS;
}
