#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libkeymap-test.h"
#include "modifiers.h"

static void
check_func(struct lk_ctx *ctx, unsigned int index, const char *expected)
{
	struct kbsentry kbs;

	memset(&kbs, 0, sizeof(kbs));
	kbs.kb_func = (unsigned char) index;

	if (lk_get_func(ctx, &kbs) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to get function string %u", index);

	if (strcmp((char *) kbs.kb_string, expected) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unexpected function string %u", index);
}

static void
test_sparse_keymaps(void)
{
	static const char keymap_text[] =
		"keymaps 0-2,4\n"
		"keycode 30 = a b c d\n";
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
}

static void
test_strings_as_usual(void)
{
	static const char keymap_text[] = "strings as usual\n";
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
}

static void
test_compose_as_usual_for_charset(void)
{
	static const char keymap_text[] =
		"charset \"iso-8859-1\"\n"
		"compose as usual for \"iso-8859-1\"\n";
	struct parsed_keymap keymap;
	struct lk_kbdiacr dcr;

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

	free_test_keymap(&keymap);
}

static void
test_charset_flag_clear(void)
{
	static const char keymap_text[] = "charset \"iso-8859-1\"\n";
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
}

static void
test_modifier_lines(void)
{
	static const char keymap_text[] =
		"plain keycode 30 = a\n"
		"shift keycode 30 = b\n"
		"control alt keycode 31 = c\n"
		"shift keycode 31 = d\n";
	struct parsed_keymap keymap;

	if (parse_test_keymap_string(&keymap, "inline-modifiers.map", keymap_text) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to parse modifier key bindings");

	if (KVAL(lk_get_key(keymap.ctx, 0, 30)) != 'a')
		kbd_error(EXIT_FAILURE, 0, "Unexpected plain key binding");

	if (KVAL(lk_get_key(keymap.ctx, 1, 30)) != 'b')
		kbd_error(EXIT_FAILURE, 0, "Unexpected shifted key binding");

	if (KVAL(lk_get_key(keymap.ctx, M_CTRL | M_ALT, 31)) != 'c')
		kbd_error(EXIT_FAILURE, 0, "Unexpected control+alt key binding");

	if (KVAL(lk_get_key(keymap.ctx, M_SHIFT, 31)) != 'd')
		kbd_error(EXIT_FAILURE, 0, "Unexpected shifted key binding after modifier reset");

	free_test_keymap(&keymap);
}

static void
test_plus_rvalues(void)
{
	static const char keymap_text[] =
		"keycode 30 = +a\n"
		"keycode 31 = +0xE9\n";
	struct parsed_keymap keymap;
	int value;

	if (parse_test_keymap_string(&keymap, "inline-plus-rvalue.map", keymap_text) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to parse +rvalue entries");

	value = lk_get_key(keymap.ctx, 0, 30);
	if (value != lk_add_capslock(keymap.ctx, K(KT_LATIN, 'a')))
		kbd_error(EXIT_FAILURE, 0, "Unexpected +literal result");

	value = lk_get_key(keymap.ctx, 0, 31);
	if (value != lk_add_capslock(keymap.ctx, 0x00e9))
		kbd_error(EXIT_FAILURE, 0, "Unexpected +number result");

	free_test_keymap(&keymap);
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	test_sparse_keymaps();
	test_strings_as_usual();
	test_compose_as_usual_for_charset();
	test_charset_flag_clear();
	test_modifier_lines();
	test_plus_rvalues();

	return EXIT_SUCCESS;
}
