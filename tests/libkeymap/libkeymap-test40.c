#include <stdio.h>
#include <stdlib.h>

#include "libkeymap-test.h"
#include "modifiers.h"

static const char keymap_text[] =
	"control alt keycode 30 = b\n"
	"shift keycode 30 = c\n";

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct parsed_keymap keymap;

	if (parse_test_keymap_string(&keymap, "inline-modifier-chain.map", keymap_text) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to parse modifier chain");

	if (KVAL(lk_get_key(keymap.ctx, M_CTRL | M_ALT, 30)) != 'b')
		kbd_error(EXIT_FAILURE, 0, "Unexpected control+alt key binding");

	if (KVAL(lk_get_key(keymap.ctx, M_SHIFT, 30)) != 'c')
		kbd_error(EXIT_FAILURE, 0, "Unexpected shifted key binding after modifier reset");

	free_test_keymap(&keymap);

	return EXIT_SUCCESS;
}
