#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "libkeymap-test.h"

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct parsed_keymap keymap;

	load_test_keymap(&keymap, "keymap0.map");

	int fake_console = fileno(keymap.file);

	lk_dump_summary(keymap.ctx, stdout, fake_console);
	lk_dump_symbols(keymap.ctx, stdout);

	fprintf(stdout, "\n");
	fprintf(stdout, "Available charsets: ");
	lk_list_charsets(stdout);

	free_test_keymap(&keymap);

	return EXIT_SUCCESS;
}
