#ifndef TESTS_LIBKEYMAP_TEST_H
#define TESTS_LIBKEYMAP_TEST_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <keymap.h>
#include "libcommon.h"

struct parsed_keymap {
	FILE *file;
	struct kbdfile *kbdfile;
	struct kbdfile_ctx *kbdfile_ctx;
	struct lk_ctx *ctx;
};

static inline void
load_test_keymap(struct parsed_keymap *keymap, const char *filename)
{
	char path[256];

	memset(keymap, 0, sizeof(*keymap));

	keymap->kbdfile_ctx = kbdfile_context_new();
	if (!keymap->kbdfile_ctx)
		kbd_error(EXIT_FAILURE, 0, "Unable to create kbdfile context");

	keymap->kbdfile = kbdfile_new(keymap->kbdfile_ctx);
	if (!keymap->kbdfile)
		kbd_error(EXIT_FAILURE, 0, "Unable to create kbdfile");

	keymap->ctx = lk_init();
	lk_set_log_fn(keymap->ctx, NULL, NULL);

	kbdfile_set_pathname(keymap->kbdfile, filename);

	snprintf(path, sizeof(path), TESTDIR "/data/libkeymap/%s", filename);

	keymap->file = fopen(path, "r");
	if (!keymap->file)
		kbd_error(EXIT_FAILURE, 0, "Unable to open: %s: %s", path, strerror(errno));

	kbdfile_set_file(keymap->kbdfile, keymap->file);

	if (lk_parse_keymap(keymap->ctx, keymap->kbdfile) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to parse keymap");
}

static inline void
free_test_keymap(struct parsed_keymap *keymap)
{
	kbdfile_free(keymap->kbdfile);
	kbdfile_context_free(keymap->kbdfile_ctx);
	lk_free(keymap->ctx);
}

#endif
