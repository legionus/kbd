#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libkeymap-test.h"

static void
test_compose_as_usual_default(void)
{
	static const char keymap_text[] = "compose as usual\n";
	struct parsed_keymap keymap;
	struct lk_kbdiacr dcr;

	if (parse_test_keymap_string(&keymap, "inline-compose-default.map", keymap_text) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to parse compose-as-usual line");

	if (!lk_diacr_exists(keymap.ctx, 0) || !lk_diacr_exists(keymap.ctx, 1))
		kbd_error(EXIT_FAILURE, 0, "Compose-as-usual did not populate accent table");

	if (lk_get_diacr(keymap.ctx, 0, &dcr) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to get first compose entry");

	if (dcr.diacr != '`' || dcr.base != 'A' || dcr.result != 0300U)
		kbd_error(EXIT_FAILURE, 0, "Unexpected first compose entry");

	free_test_keymap(&keymap);
}

static void
test_string_escapes(void)
{
	static const char keymap_text[] = "string F1 = \"A\\n\\\"\\\\\\123\"\n";
	const char expected[] = "A\n\"\\S";
	struct parsed_keymap keymap;
	struct kbsentry kbs;

	if (parse_test_keymap_string(&keymap, "inline-string-escapes.map", keymap_text) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to parse string escapes");

	memset(&kbs, 0, sizeof(kbs));
	kbs.kb_func = 0;

	if (lk_get_func(keymap.ctx, &kbs) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to get parsed function string");

	if (strcmp((char *)kbs.kb_string, expected) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unexpected parsed function string");

	free_test_keymap(&keymap);
}

static void
test_compose_literals(void)
{
	static const char keymap_text[] =
		"compose U+0060 U+0061 to U+00E9\n"
		"compose '\\033' 'o' to 'x'\n";
	struct parsed_keymap keymap;
	struct lk_kbdiacr dcr;

	if (parse_test_keymap_string(&keymap, "inline-compose-literals.map", keymap_text) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to parse compose literals");

	if (lk_get_diacr(keymap.ctx, 0, &dcr) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to get first compose entry");

	if (dcr.diacr != '`' || dcr.base != 'a' || dcr.result != 0x00e9U)
		kbd_error(EXIT_FAILURE, 0, "Unexpected unicode compose entry");

	if (lk_get_diacr(keymap.ctx, 1, &dcr) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to get second compose entry");

	if (dcr.diacr != 033U || dcr.base != 'o' || dcr.result != 'x')
		kbd_error(EXIT_FAILURE, 0, "Unexpected char-literal compose entry");

	free_test_keymap(&keymap);
}

static void
test_line_continuation(void)
{
	static const char keymap_text[] =
		"# comment before continued line\n"
		"keycode 30 = a \\\n"
		"b # trailing comment\n";
	struct parsed_keymap keymap;

	if (parse_test_keymap_string(&keymap, "inline-continuation.map", keymap_text) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to parse continued keycode line");

	if (KVAL(lk_get_key(keymap.ctx, 0, 30)) != 'a')
		kbd_error(EXIT_FAILURE, 0, "Unexpected first continued binding");

	if (KVAL(lk_get_key(keymap.ctx, 1, 30)) != 'b')
		kbd_error(EXIT_FAILURE, 0, "Unexpected second continued binding");

	free_test_keymap(&keymap);
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	test_compose_as_usual_default();
	test_string_escapes();
	test_compose_literals();
	test_line_continuation();

	return EXIT_SUCCESS;
}
