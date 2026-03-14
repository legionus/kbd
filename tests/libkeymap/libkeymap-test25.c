#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <linux/kd.h>
#include <linux/keyboard.h>

#include <keymap.h>
#include "contextP.h"
#include "libcommon.h"

#define TEST_CONSOLE_FD 89
#define TEST_KEY_TABLE 0
#define TEST_KEY_INDEX 12
#define TEST_KEY_VALUE K(KT_LATIN, 'u')

static bool saw_unicode_mode;
static bool saw_key_binding;
static bool saw_restore_mode;
static int unicode_mode_calls;
static int restore_mode_calls;

static int
fake_ioctl(int fd, unsigned long req, uintptr_t arg)
{
	if (fd != TEST_CONSOLE_FD)
		kbd_error(EXIT_FAILURE, 0, "unexpected fd: %d", fd);

	switch (req) {
		case KDSKBMODE: {
			int mode = (int)arg;

			if (mode == K_UNICODE) {
				if (saw_unicode_mode || saw_key_binding || saw_restore_mode)
					kbd_error(EXIT_FAILURE, 0, "unexpected Unicode mode switch order");

				saw_unicode_mode = true;
				unicode_mode_calls++;
				return 0;
			}

			if (mode == K_XLATE) {
				if (!saw_unicode_mode || !saw_key_binding || saw_restore_mode)
					kbd_error(EXIT_FAILURE, 0, "unexpected keyboard mode restore order");

				saw_restore_mode = true;
				restore_mode_calls++;
				return 0;
			}

			kbd_error(EXIT_FAILURE, 0, "unexpected keyboard mode: %d", mode);
			return -1;
		}

		case KDSKBENT: {
			struct kbentry *ke = (struct kbentry *)arg;

			if (!saw_unicode_mode || saw_restore_mode)
				kbd_error(EXIT_FAILURE, 0, "unexpected key binding order");

			if (ke->kb_table != TEST_KEY_TABLE)
				kbd_error(EXIT_FAILURE, 0, "unexpected key table: %u", ke->kb_table);

			if (ke->kb_index != TEST_KEY_INDEX)
				kbd_error(EXIT_FAILURE, 0, "unexpected key index: %u", ke->kb_index);

			if (ke->kb_value != TEST_KEY_VALUE)
				kbd_error(EXIT_FAILURE, 0, "unexpected key value: %u", ke->kb_value);

			saw_key_binding = true;
			return 0;
		}
	}

	kbd_error(EXIT_FAILURE, 0, "unexpected request: %lu", req);
	return -1;
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
		kbd_error(EXIT_FAILURE, 0, "Unable to add key for Unicode mode test");

	if (lk_set_parser_flags(ctx, LK_FLAG_UNICODE_MODE) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to enable Unicode mode loading");

	ops = ctx->ops;
	ops.ioctl_fn = fake_ioctl;
	lk_set_ops(ctx, &ops);

	if (lk_load_keymap(ctx, TEST_CONSOLE_FD, K_XLATE) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to load keymap in Unicode mode");

	if (!saw_unicode_mode)
		kbd_error(EXIT_FAILURE, 0, "Keyboard was not switched to Unicode mode");

	if (!saw_key_binding)
		kbd_error(EXIT_FAILURE, 0, "Key binding was not loaded");

	if (!saw_restore_mode)
		kbd_error(EXIT_FAILURE, 0, "Keyboard mode was not restored");

	if (unicode_mode_calls != 1)
		kbd_error(EXIT_FAILURE, 0, "Unexpected number of Unicode mode switches: %d",
			unicode_mode_calls);

	if (restore_mode_calls != 1)
		kbd_error(EXIT_FAILURE, 0, "Unexpected number of keyboard mode restores: %d",
			restore_mode_calls);

	lk_free(ctx);

	return EXIT_SUCCESS;
}
