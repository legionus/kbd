#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libkeymap-test.h"

static char *
dump_canonical_keymap(struct parsed_keymap *keymap)
{
	char *buf = NULL;
	size_t size = 0;
	FILE *fp;

	if (lk_add_constants(keymap->ctx) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to add key constants");

	fp = open_memstream(&buf, &size);
	if (!fp)
		kbd_error(EXIT_FAILURE, 0, "Unable to create memory stream");

	lk_dump_keymap(keymap->ctx, fp, LK_SHAPE_DEFAULT, 0);
	lk_dump_diacs(keymap->ctx, fp);

	fclose(fp);
	return buf;
}

static void
expect_round_trip_string(const char *pathname, const char *content)
{
	struct parsed_keymap first;
	struct parsed_keymap second;
	char *first_dump;
	char *second_dump;

	if (parse_test_keymap_string(&first, pathname, content) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to parse initial keymap string: %s", pathname);

	first_dump = dump_canonical_keymap(&first);

	if (parse_test_keymap_string(&second, pathname, first_dump) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to parse canonical keymap dump: %s", pathname);

	second_dump = dump_canonical_keymap(&second);

	if (strcmp(first_dump, second_dump) != 0)
		kbd_error(EXIT_FAILURE, 0, "Round-trip mismatch for %s", pathname);

	free(second_dump);
	free(first_dump);
	free_test_keymap(&second);
	free_test_keymap(&first);
}

static void
expect_round_trip_file(const char *path)
{
	struct parsed_keymap first;
	struct parsed_keymap second;
	char *first_dump;
	char *second_dump;

	init_test_keymap(&first, path);
	first.file = fopen(path, "r");
	if (!first.file)
		kbd_error(EXIT_FAILURE, 0, "Unable to open %s", path);

	if (parse_test_keymap_stream(&first, first.file) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to parse file-backed keymap %s", path);

	first_dump = dump_canonical_keymap(&first);

	if (parse_test_keymap_string(&second, path, first_dump) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to parse canonical dump for %s", path);

	second_dump = dump_canonical_keymap(&second);

	if (strcmp(first_dump, second_dump) != 0)
		kbd_error(EXIT_FAILURE, 0, "Round-trip mismatch for %s", path);

	free(second_dump);
	free(first_dump);
	free_test_keymap(&second);
	free_test_keymap(&first);
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	/* Basic canonicalization for plain sequential key bindings. */
	expect_round_trip_string("keymap0.map",
				 "keycode 16 = q\nkeycode 17 = w\nkeycode 18 = e\n");

	/* Sparse keymaps ranges should stay stable through dump/reparse. */
	expect_round_trip_string("inline-keymaps.map",
				 "keymaps 0-2,4\n"
				 "keycode 30 = a b c d\n");

	/* alt_is_meta should survive alongside explicit meta-capable bindings. */
	expect_round_trip_string("inline-alt-is-meta.map",
				 "alt_is_meta\n"
				 "keycode 30 = a A Meta_a\n");

	/* Default compose rules should round-trip in canonical dump form. */
	expect_round_trip_string("keymap7.map", "compose as usual\n");

	/* Charset-gated default compose rules should stay stable. */
	expect_round_trip_string("inline-compose-usual-charset.map",
				 "charset \"iso-8859-1\"\n"
				 "compose as usual for \"iso-8859-1\"\n");

	/* Repeated charset directives should dump and reparse consistently. */
	expect_round_trip_string("charset-keymap0.map",
				 "charset \"koi8-r\"\ncharset \"iso-8859-1\"\ncharset \"iso-8859-2\"\n");

	/* Explicit function-string assignments should remain canonical. */
	expect_round_trip_string("keymap5.map",
				 "string F1 = \"FUNCTION1\"\nstring F2 = \"FUNCTION2\"\n");

	/* Built-in default strings should survive dump/reparse unchanged. */
	expect_round_trip_string("inline-strings-usual.map", "strings as usual\n");

	/* Default strings plus explicit overrides should remain stable. */
	expect_round_trip_string("inline-strings-override.map",
				 "strings as usual\n"
				 "string F1 = \"OVERRIDE\"\n"
				 "string F2 = \"SECOND\"\n");

	/* Mixed modifier lines should preserve map selection and reset semantics. */
	expect_round_trip_string("inline-modifiers.map",
				 "plain keycode 30 = a\n"
				 "shift keycode 30 = b\n"
				 "control alt keycode 31 = c\n"
				 "shift altgr keycode 31 = d\n");

	/* +rvalue normalization should be stable for literal and numeric inputs. */
	expect_round_trip_string("inline-plus-rvalue.map",
				 "keycode 30 = +a\n"
				 "keycode 31 = +0x00e9\n");

	/* Mixed literal, unicode and +capslock inputs should canonicalize. */
	expect_round_trip_string("inline-mixed-rvalues.map",
				 "keycode 30 = a\n"
				 "keycode 31 = U+00e9\n"
				 "keycode 32 = +U+00e9\n"
				 "keycode 33 = +a\n");

	/* Explicit compose rules should remain stable across syntax forms. */
	expect_round_trip_string("inline-compose-explicit.map",
				 "compose U+0060 U+0061 to U+00E0\n"
				 "compose U+007e U+006e to U+00F1\n");

	/* Default compose rules plus explicit additions should round-trip. */
	expect_round_trip_string("inline-compose-usual-plus-explicit.map",
				 "compose as usual\n"
				 "compose '~' 'n' to U+00f1\n");

	/* Escaped function strings should preserve quoting and control bytes. */
	expect_round_trip_string("inline-string-escapes.map",
				 "string F5 = \"\\\"\\\\\\012\"\n"
				 "string F6 = \"A\\033B\"\n");

	/* A mixed parser grammar example should survive as one canonical document. */
	expect_round_trip_string("inline-kitchen-sink.map",
				 "charset \"iso-8859-1\"\n"
				 "alt_is_meta\n"
				 "strings as usual\n"
				 "compose as usual for \"iso-8859-1\"\n"
				 "string F1 = \"OVERRIDE\"\n"
				 "keymaps 0-1,4\n"
				 "keycode 30 = a A Meta_a\n"
				 "compose U+007E U+006E to U+00F1\n");

	/* A real file-backed keymap should produce a stable canonical dump. */
	expect_round_trip_file(TESTDIR "/data/keymaps/i386/qwerty/us.map");

	return EXIT_SUCCESS;
}
