#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <linux/kd.h>
#include <linux/keyboard.h>

#include <keymap.h>
#include "contextP.h"
#include "libcommon.h"

#define TEST_CONSOLE_FD 103
#define TEST_KEY_TABLE 0
#define TEST_KEY_INDEX 6
#define TEST_KEY_VALUE K(KT_LATIN, 'i')

static int key_binding_attempts;
static bool saw_function_ioctls;
static bool saw_diacr_ioctls;

static int
fake_ioctl(int fd, unsigned long req, uintptr_t arg)
{
	if (fd != TEST_CONSOLE_FD)
		kbd_error(EXIT_FAILURE, 0, "unexpected fd: %d", fd);

	switch (req) {
		case KDSKBENT: {
			struct kbentry *ke = (struct kbentry *)arg;

			key_binding_attempts++;

			if (ke->kb_table != TEST_KEY_TABLE)
				kbd_error(EXIT_FAILURE, 0, "unexpected key table: %u", ke->kb_table);

			if (ke->kb_index != TEST_KEY_INDEX)
				kbd_error(EXIT_FAILURE, 0, "unexpected key index: %u", ke->kb_index);

			if (ke->kb_value != TEST_KEY_VALUE)
				kbd_error(EXIT_FAILURE, 0, "unexpected key value: %u", ke->kb_value);

			errno = EIO;
			return -1;
		}

		case KDSKBSENT:
			saw_function_ioctls = true;
			return 0;

		case KDSKBDIACR:
		case KDSKBDIACRUC:
			saw_diacr_ioctls = true;
			return 0;
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
		kbd_error(EXIT_FAILURE, 0, "Unable to add key for EIO test");

	ops = ctx->ops;
	ops.ioctl_fn = fake_ioctl;
	lk_set_ops(ctx, &ops);

	if (lk_load_keymap(ctx, TEST_CONSOLE_FD, K_XLATE) == 0)
		kbd_error(EXIT_FAILURE, 0, "EIO did not abort keymap load");

	if (key_binding_attempts != 1)
		kbd_error(EXIT_FAILURE, 0, "Unexpected number of key binding attempts: %d",
			key_binding_attempts);

	if (saw_function_ioctls)
		kbd_error(EXIT_FAILURE, 0, "Function strings were loaded after EIO");

	if (saw_diacr_ioctls)
		kbd_error(EXIT_FAILURE, 0, "Compose table was loaded after EIO");

	lk_free(ctx);

	return EXIT_SUCCESS;
}
