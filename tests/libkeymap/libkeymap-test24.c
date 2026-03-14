#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <linux/kd.h>
#include <linux/keyboard.h>

#include <keymap.h>
#include "contextP.h"
#include "libcommon.h"

#define TEST_CONSOLE_FD 83
#define TEST_MANUAL_CLEAR_TABLE 1
#define TEST_KEY_TABLE 0
#define TEST_KEY_INDEX 9
#define TEST_KEY_VALUE K(KT_LATIN, 'x')

static bool table_cleared[NR_KEYS];

static int
fake_ioctl(int fd, unsigned long req, uintptr_t arg)
{
	struct kbentry *ke = (struct kbentry *)arg;

	if (fd != TEST_CONSOLE_FD)
		kbd_error(EXIT_FAILURE, 0, "unexpected fd: %d", fd);

	if (req != KDSKBENT)
		kbd_error(EXIT_FAILURE, 0, "unexpected request: %lu", req);

	if (ke->kb_table == TEST_KEY_TABLE) {
		if (ke->kb_index != TEST_KEY_INDEX)
			kbd_error(EXIT_FAILURE, 0, "unexpected key index for loaded table: %u",
				ke->kb_index);

		if (ke->kb_value != TEST_KEY_VALUE)
			kbd_error(EXIT_FAILURE, 0, "unexpected key value for loaded table: %u",
				ke->kb_value);

		return 0;
	}

	if (ke->kb_table == TEST_MANUAL_CLEAR_TABLE) {
		if (ke->kb_value == K_NOSUCHMAP && ke->kb_index == 0) {
			errno = EINVAL;
			return -1;
		}

		if (ke->kb_value != K_HOLE)
			kbd_error(EXIT_FAILURE, 0, "unexpected clear value for table %u index %u",
				ke->kb_table, ke->kb_index);

		table_cleared[ke->kb_index] = true;
		return 0;
	}

	if (ke->kb_index != 0)
		kbd_error(EXIT_FAILURE, 0, "unexpected nonzero index %u for table %u",
			ke->kb_index, ke->kb_table);

	if (ke->kb_value != K_NOSUCHMAP)
		kbd_error(EXIT_FAILURE, 0, "unexpected deallocate value for table %u",
			ke->kb_table);

	return 0;
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct lk_ctx *ctx;
	struct lk_ops ops;

	ctx = lk_init();
	if (!ctx)
		kbd_error(EXIT_FAILURE, 0, "Unable to initialize structure by valid pointer");

	lk_set_log_fn(ctx, NULL, NULL);

	if (lk_add_key(ctx, TEST_KEY_TABLE, TEST_KEY_INDEX, TEST_KEY_VALUE) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to add key for loaded table");

	if (lk_set_keywords(ctx, LK_KEYWORD_KEYMAPS) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to enable keymap clearing");

	ops = ctx->ops;
	ops.ioctl_fn = fake_ioctl;
	lk_set_ops(ctx, &ops);

	if (lk_load_keymap(ctx, TEST_CONSOLE_FD, K_XLATE) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to load keymap with manual clear fallback");

	for (int i = 0; i < NR_KEYS; i++) {
		if (!table_cleared[i])
			kbd_error(EXIT_FAILURE, 0, "Table %d index %d was not cleared by hand",
				TEST_MANUAL_CLEAR_TABLE, i);
	}

	lk_free(ctx);

	return EXIT_SUCCESS;
}
