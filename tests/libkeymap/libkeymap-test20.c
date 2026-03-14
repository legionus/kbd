#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "libkeymap-test.h"

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct parsed_keymap keymap;
	struct lk_kbdiacr dcr;

	load_test_keymap(&keymap, "keymap7.map");

	lk_dump_diacs(keymap.ctx, stdout);

	if (!lk_diacr_exists(keymap.ctx, 1))
		kbd_error(EXIT_FAILURE, 0, "Unable to check existence of diacr with index 1");

	if (lk_get_diacr(keymap.ctx, 1, &dcr) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to get of diacr with index 1");

	if (dcr.diacr != '`' || dcr.base != 'a' || dcr.result != 0340)
		kbd_error(EXIT_FAILURE, 0, "Unexpected diacr with index 1");

	if (lk_del_diacr(keymap.ctx, 1) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to remove diacr with index 1");

	if (lk_diacr_exists(keymap.ctx, 1))
		kbd_error(EXIT_FAILURE, 0, "Removed diacr with index 1 still exists");

	if (lk_add_diacr(keymap.ctx, 1, &dcr) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to add diacr with index 1");

	free_test_keymap(&keymap);

	return EXIT_SUCCESS;
}
