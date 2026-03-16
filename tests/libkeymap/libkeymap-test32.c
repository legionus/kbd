#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libkeymap-test.h"

static void
expect_parse_failure(const char *pathname, const char *content, const char *what)
{
	struct parsed_keymap keymap;

	if (parse_test_keymap_string(&keymap, pathname, content) == 0)
		kbd_error(EXIT_FAILURE, 0, "Unexpected success for %s", what);

	free_test_keymap(&keymap);
}

static void
test_non_function_string_rejected(void)
{
	struct parsed_keymap keymap;
	static const char keymap_text[] = "string a = \"not-a-function\"\n";

	if (parse_test_keymap_string(&keymap, "inline-bad-string.map", keymap_text) == 0)
		kbd_error(EXIT_FAILURE, 0, "Unexpected success for non-function string symbol");

	if (lk_func_exists(keymap.ctx, 0))
		kbd_error(EXIT_FAILURE, 0, "Parser stored a function string after rejecting input");

	free_test_keymap(&keymap);
}

static void
test_unknown_charset_rejected(void)
{
	struct parsed_keymap keymap;
	static const char keymap_text[] = "charset \"definitely-not-a-charset\"\n";

	if (parse_test_keymap_string(&keymap, "inline-bad-charset.map", keymap_text) == 0)
		kbd_error(EXIT_FAILURE, 0, "Unexpected success for unknown charset");

	if (strcmp(lk_get_charset(keymap.ctx), "iso-8859-1") != 0)
		kbd_error(EXIT_FAILURE, 0, "Unknown charset changed the default charset");

	if (lk_get_keywords(keymap.ctx) & LK_KEYWORD_CHARSET)
		kbd_error(EXIT_FAILURE, 0, "Unknown charset unexpectedly set the charset keyword");

	free_test_keymap(&keymap);
}

static void
test_unsupported_compose_charset_rejected(void)
{
	struct parsed_keymap keymap;
	static const char keymap_text[] = "compose as usual for \"koi8-r\"\n";

	if (parse_test_keymap_string(&keymap, "inline-bad-compose.map", keymap_text) == 0)
		kbd_error(EXIT_FAILURE, 0, "Unexpected success for unsupported compose charset");

	if (lk_diacr_exists(keymap.ctx, 0))
		kbd_error(EXIT_FAILURE, 0, "Parser stored compose rules after rejecting charset");

	free_test_keymap(&keymap);
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	expect_parse_failure("inline-parse-error.map",
			     "keymaps 0-\n",
			     "malformed keymaps line");
	test_non_function_string_rejected();
	test_unknown_charset_rejected();
	test_unsupported_compose_charset_rejected();
	expect_parse_failure("inline-too-many-rvalues.map",
			     "keymaps 0-1\nkeycode 30 = a b c\n",
			     "extra rvalues for active keymaps");

	return EXIT_SUCCESS;
}
