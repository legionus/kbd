#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libkeymap-test.h"

static const char keymap_text[] =
	"charset \"iso-8859-1\"\n"
	"compose as usual for \"iso-8859-1\"\n";

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct lk_kbdiacr dcr;
	struct parsed_keymap keymap;

	if (parse_test_keymap_string(&keymap, "inline-compose.map", keymap_text) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to parse inline compose rules");

	if (!(lk_get_keywords(keymap.ctx) & LK_KEYWORD_CHARSET))
		kbd_error(EXIT_FAILURE, 0, "The charset keyword was not recorded");

	if (strcmp(lk_get_charset(keymap.ctx), "iso-8859-1") != 0)
		kbd_error(EXIT_FAILURE, 0, "Unexpected charset after parsing compose rules");

	if (!lk_diacr_exists(keymap.ctx, 0) || !lk_diacr_exists(keymap.ctx, 1))
		kbd_error(EXIT_FAILURE, 0, "Unable to populate compose table");

	if (lk_get_diacr(keymap.ctx, 0, &dcr) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to get first compose entry");

	if (dcr.diacr != '`' || dcr.base != 'A' || dcr.result != 0300U)
		kbd_error(EXIT_FAILURE, 0, "Unexpected first compose entry");

	if (lk_get_diacr(keymap.ctx, 1, &dcr) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to get second compose entry");

	if (dcr.diacr != '`' || dcr.base != 'a' || dcr.result != 0340U)
		kbd_error(EXIT_FAILURE, 0, "Unexpected second compose entry");

	free_test_keymap(&keymap);

	return EXIT_SUCCESS;
}
