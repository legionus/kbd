#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libkeymap-test.h"

static const char keymap_text[] = "charset \"iso-8859-1\"\n";

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct parsed_keymap keymap;

	init_test_keymap(&keymap, "inline-charset-flags.map");

	if (lk_set_parser_flags(keymap.ctx, LK_FLAG_PREFER_UNICODE) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to enable prefer-unicode before parse");

	set_test_keymap_string(&keymap, keymap_text);

	if (parse_test_keymap_stream(&keymap, keymap.file) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to parse charset line");

	if (strcmp(lk_get_charset(keymap.ctx), "iso-8859-1") != 0)
		kbd_error(EXIT_FAILURE, 0, "Unexpected charset after parsing");

	if (lk_get_parser_flags(keymap.ctx) & LK_FLAG_PREFER_UNICODE)
		kbd_error(EXIT_FAILURE, 0, "The parser kept prefer-unicode for iso-8859-1");

	free_test_keymap(&keymap);

	return EXIT_SUCCESS;
}
