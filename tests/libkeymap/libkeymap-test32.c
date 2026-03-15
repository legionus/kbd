#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libkeymap-test.h"

static const char keymap_text[] = "strings as usual\n";

static void
check_func(struct lk_ctx *ctx, unsigned int index, const char *expected)
{
	struct kbsentry kbs;

	memset(&kbs, 0, sizeof(kbs));
	kbs.kb_func = (unsigned char)index;

	if (lk_get_func(ctx, &kbs) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to get function string %u", index);

	if (strcmp((char *)kbs.kb_string, expected) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unexpected function string %u", index);
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct parsed_keymap keymap;

	if (parse_test_keymap_string(&keymap, "inline-strings.map", keymap_text) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to parse inline strings");

	if (!(lk_get_keywords(keymap.ctx) & LK_KEYWORD_STRASUSUAL))
		kbd_error(EXIT_FAILURE, 0, "The strings-as-usual keyword was not recorded");

	check_func(keymap.ctx, 0, "\033[[A");
	check_func(keymap.ctx, 25, "\033[6~");
	check_func(keymap.ctx, 255, "\033[Z");

	if (lk_func_exists(keymap.ctx, 26))
		kbd_error(EXIT_FAILURE, 0, "Unexpected empty default function string");

	free_test_keymap(&keymap);

	return EXIT_SUCCESS;
}
