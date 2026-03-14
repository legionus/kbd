#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <linux/keyboard.h>

#include <keymap.h>
#include "contextP.h"
#include "libcommon.h"

#define TEST_CONSOLE_FD 79

static int
fake_ioctl(int fd, unsigned long req, uintptr_t arg)
{
	struct kbentry *ke = (struct kbentry *)arg;

	if (fd != TEST_CONSOLE_FD)
		kbd_error(EXIT_FAILURE, 0, "unexpected fd: %d", fd);

	if (req != KDGKBENT)
		kbd_error(EXIT_FAILURE, 0, "unexpected request: %lu", req);

	if (ke->kb_table == 0) {
		switch (ke->kb_index) {
			case 0:
				ke->kb_value = K(KT_LATIN, 'q');
				break;
			case 1:
				ke->kb_value = K(KT_LATIN, 'w');
				break;
			default:
				ke->kb_value = K_HOLE;
				break;
		}
		return 0;
	}

	if (ke->kb_index == 0) {
		ke->kb_value = K_NOSUCHMAP;
		return 0;
	}

	kbd_error(EXIT_FAILURE, 0, "unexpected nonzero index %u for table %u",
		ke->kb_index, ke->kb_table);
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

	ops = ctx->ops;
	ops.ioctl_fn = fake_ioctl;
	lk_set_ops(ctx, &ops);

	if (lk_kernel_keys(ctx, TEST_CONSOLE_FD) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to load keymap entries from kernel");

	if (!lk_map_exists(ctx, 0))
		kbd_error(EXIT_FAILURE, 0, "Keymap 0 was not loaded");

	if (lk_map_exists(ctx, 1))
		kbd_error(EXIT_FAILURE, 0, "Unexpected keymap 1 was loaded");

	if (lk_get_key(ctx, 0, 0) != K(KT_LATIN, 'q'))
		kbd_error(EXIT_FAILURE, 0, "Unexpected key value for table 0 index 0");

	if (lk_get_key(ctx, 0, 1) != K(KT_LATIN, 'w'))
		kbd_error(EXIT_FAILURE, 0, "Unexpected key value for table 0 index 1");

	if (lk_get_key(ctx, 0, 2) != K_HOLE)
		kbd_error(EXIT_FAILURE, 0, "Unexpected key value for table 0 index 2");

	lk_free(ctx);

	return EXIT_SUCCESS;
}
