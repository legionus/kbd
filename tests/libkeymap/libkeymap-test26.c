#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <linux/kd.h>
#include <linux/keyboard.h>

#include <keymap.h>
#include "contextP.h"
#include "libcommon.h"

#define TEST_CONSOLE_FD 97
#define TEST_KEY_TABLE 0
#define TEST_KEY_INDEX 3
#define TEST_KEY_VALUE K(KT_LATIN, 'z')

static bool saw_unicode_mode_attempt;
static bool saw_key_binding;
static bool saw_restore_mode;

static int
fake_ioctl(int fd, unsigned long req, uintptr_t arg)
{
	if (fd != TEST_CONSOLE_FD)
		kbd_error(EXIT_FAILURE, 0, "unexpected fd: %d", fd);

	switch (req) {
		case KDSKBMODE:
			if ((int)arg != K_UNICODE)
				kbd_error(EXIT_FAILURE, 0, "unexpected keyboard mode request: %d",
					(int)arg);

			saw_unicode_mode_attempt = true;
			errno = EINVAL;
			return -1;

		case KDSKBENT:
			saw_key_binding = true;
			return 0;
	}

	saw_restore_mode = true;
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
		kbd_error(EXIT_FAILURE, 0, "Unable to add key for Unicode mode failure test");

	if (lk_set_parser_flags(ctx, LK_FLAG_UNICODE_MODE) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to enable Unicode mode loading");

	ops = ctx->ops;
	ops.ioctl_fn = fake_ioctl;
	lk_set_ops(ctx, &ops);

	if (lk_load_keymap(ctx, TEST_CONSOLE_FD, K_XLATE) == 0)
		kbd_error(EXIT_FAILURE, 0, "Unicode mode failure did not abort keymap load");

	if (!saw_unicode_mode_attempt)
		kbd_error(EXIT_FAILURE, 0, "Keyboard was not switched to Unicode mode");

	if (saw_key_binding)
		kbd_error(EXIT_FAILURE, 0, "Key binding was loaded after Unicode mode failure");

	if (saw_restore_mode)
		kbd_error(EXIT_FAILURE, 0, "Keyboard mode restore happened after Unicode mode failure");

	lk_free(ctx);

	return EXIT_SUCCESS;
}
