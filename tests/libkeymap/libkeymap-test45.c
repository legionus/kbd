#include <stdio.h>
#include <stdlib.h>

#include <linux/kd.h>
#include <linux/keyboard.h>

#include <keymap.h>
#include "contextP.h"
#include "libcommon.h"

#define TEST_CONSOLE_FD 101

static unsigned int diacr_calls;

static int
fake_ioctl(int fd, unsigned long req, uintptr_t arg)
{
	struct kbdiacrsuc *kdu = (struct kbdiacrsuc *)arg;

	if (fd != TEST_CONSOLE_FD)
		kbd_error(EXIT_FAILURE, 0, "unexpected fd: %d", fd);

	if (req != KDSKBDIACRUC)
		kbd_error(EXIT_FAILURE, 0, "unexpected request: %lu", req);

	if (kdu->kb_cnt != MAX_DIACR - 1)
		kbd_error(EXIT_FAILURE, 0, "unexpected accent count: %u", kdu->kb_cnt);

	diacr_calls++;
	return 0;
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct lk_ctx *ctx;
	struct lk_ops ops;
	struct lk_kbdiacr dcr;

	ctx = lk_init();
	if (!ctx)
		kbd_error(EXIT_FAILURE, 0, "Unable to initialize structure by valid pointer");

	lk_set_log_fn(ctx, NULL, NULL);

	if (lk_set_parser_flags(ctx, LK_FLAG_PREFER_UNICODE) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to enable Unicode compose loading");

	for (unsigned int i = 0; i < MAX_DIACR; i++) {
		dcr.diacr = 0x1000u + i;
		dcr.base = 0x1100u + i;
		dcr.result = 0x1200u + i;

		if (lk_add_diacr(ctx, (int) i, &dcr) != 0)
			kbd_error(EXIT_FAILURE, 0, "Unable to add compose entry %u", i);
	}

	ops = ctx->ops;
	ops.ioctl_fn = fake_ioctl;
	lk_set_ops(ctx, &ops);

	if (lk_load_keymap(ctx, TEST_CONSOLE_FD, K_UNICODE) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to load compose definitions at MAX_DIACR boundary");

	if (diacr_calls != 1U)
		kbd_error(EXIT_FAILURE, 0, "unexpected compose ioctl count: %u", diacr_calls);

	lk_free(ctx);

	return EXIT_SUCCESS;
}
