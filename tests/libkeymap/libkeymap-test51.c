#include <stdio.h>
#include <stdlib.h>

#include "libkeymap-test.h"

static void
expect_parse_failure(const char *pathname, const char *content, const char *what)
{
	struct parsed_keymap keymap;

	if (parse_test_keymap_string(&keymap, pathname, content) == 0)
		kbd_error(EXIT_FAILURE, 0, "Unexpected success for %s", what);

	free_test_keymap(&keymap);
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	expect_parse_failure("inline-bad-include.map",
			     "include child.map\n",
			     "include without quotes");
	expect_parse_failure("inline-bad-unicode.map",
			     "keycode 30 = U+F000\n",
			     "out-of-range unicode keysym");
	expect_parse_failure("inline-bad-include-quote.map",
			     "include \"child.map\n",
			     "unterminated include filename");
	expect_parse_failure("inline-bad-string-escape.map",
			     "string F1 = \"\\777\"\n",
			     "invalid string escape");

	return EXIT_SUCCESS;
}
