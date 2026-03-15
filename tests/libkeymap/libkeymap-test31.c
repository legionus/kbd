#include <stdio.h>
#include <stdlib.h>

#include "libkeymap-test.h"

static const char keymap_text[] =
	"keymaps 0-2,4\n"
	"keycode 30 = a b c d\n";

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct parsed_keymap keymap;

	if (parse_test_keymap_string(&keymap, "inline-keymaps.map", keymap_text) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to parse inline keymap");

	if (!(lk_get_keywords(keymap.ctx) & LK_KEYWORD_KEYMAPS))
		kbd_error(EXIT_FAILURE, 0, "The keymaps keyword was not recorded");

	if (!lk_map_exists(keymap.ctx, 0) || !lk_map_exists(keymap.ctx, 1) ||
	    !lk_map_exists(keymap.ctx, 2) || !lk_map_exists(keymap.ctx, 4))
		kbd_error(EXIT_FAILURE, 0, "Unable to allocate maps from keymaps range");

	if (lk_map_exists(keymap.ctx, 3))
		kbd_error(EXIT_FAILURE, 0, "Unexpected map created from sparse keymaps range");

	if (KVAL(lk_get_key(keymap.ctx, 0, 30)) != 'a' ||
	    KVAL(lk_get_key(keymap.ctx, 1, 30)) != 'b' ||
	    KVAL(lk_get_key(keymap.ctx, 2, 30)) != 'c' ||
	    KVAL(lk_get_key(keymap.ctx, 4, 30)) != 'd')
		kbd_error(EXIT_FAILURE, 0, "Unexpected key values after parsing keymaps line");

	free_test_keymap(&keymap);

	return EXIT_SUCCESS;
}
