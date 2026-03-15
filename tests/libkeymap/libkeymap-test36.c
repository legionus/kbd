#include <stdio.h>
#include <stdlib.h>

#include "libkeymap-test.h"

static const char keymap_text[] =
	"plain keycode 30 = a\n"
	"shift keycode 30 = b\n";

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct parsed_keymap keymap;

	if (parse_test_keymap_string(&keymap, "inline-modifiers.map", keymap_text) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to parse modifier key bindings");

	if (KVAL(lk_get_key(keymap.ctx, 0, 30)) != 'a')
		kbd_error(EXIT_FAILURE, 0, "Unexpected plain key binding");

	if (KVAL(lk_get_key(keymap.ctx, 1, 30)) != 'b')
		kbd_error(EXIT_FAILURE, 0, "Unexpected shifted key binding");

	free_test_keymap(&keymap);

	return EXIT_SUCCESS;
}
