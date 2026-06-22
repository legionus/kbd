#include <stdio.h>
#include <stdlib.h>

#include "contextP.h"
#include "libkeymap-test.h"

static int
parse_feature_keymap(struct parsed_keymap *keymap, enum lk_feature_status status)
{
	char path[256];

	init_test_keymap(keymap, "keymap-feature.map");
	keymap->ctx->features[LK_FEATURE_KT_CSI] = status;

	snprintf(path, sizeof(path), TESTDIR "/data/libkeymap/keymap-feature.map");
	keymap->file = fopen(path, "r");
	if (!keymap->file)
		kbd_error(EXIT_FAILURE, 0, "Unable to open: %s", path);

	return parse_test_keymap_stream(keymap, keymap->file);
}

static void
expect_key(enum lk_feature_status status, int expected)
{
	struct parsed_keymap keymap;

	if (parse_feature_keymap(&keymap, status) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to parse conditional keymap");
	if (lk_get_key(keymap.ctx, 0, 16) != expected)
		kbd_error(EXIT_FAILURE, 0, "Conditional include selected the wrong keymap");

	free_test_keymap(&keymap);
}

static void
expect_parse_error(const char *content)
{
	struct parsed_keymap keymap;

	if (parse_test_keymap_string(&keymap, "feature-error.map", content) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to initialize conditional keymap");
	if (parse_test_keymap_stream(&keymap, keymap.file) == 0)
		kbd_error(EXIT_FAILURE, 0, "Invalid feature condition was accepted");

	free_test_keymap(&keymap);
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	setenv("LOADKEYS_INCLUDE_PATH", TESTDIR "/data/libkeymap", 1);

	expect_key(LK_FEATURE_SUPPORTED, 'q');
	expect_key(LK_FEATURE_UNSUPPORTED, 'w');
	expect_parse_error("include-if feature KT_CSI \"unused\"\n");
	expect_parse_error("include-if feature (DOES_NOT_EXIST) \"unused\"\n");

	return EXIT_SUCCESS;
}
