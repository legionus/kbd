#include <stdio.h>
#include <stdlib.h>

#include <linux/kd.h>

#include <keymap.h>
#include "contextP.h"
#include "libcommon.h"

#define TEST_CONSOLE_FD 101

static unsigned int diacr_calls;

static int
fake_ioctl(int fd, unsigned long req, uintptr_t arg)
{
	struct kbdiacrs *kd = (struct kbdiacrs *)arg;

	if (fd != TEST_CONSOLE_FD)
		kbd_error(EXIT_FAILURE, 0, "unexpected fd: %d", fd);

	if (req != KDSKBDIACR)
		kbd_error(EXIT_FAILURE, 0, "unexpected request: %lu", req);

	if (kd->kb_cnt != 2)
		kbd_error(EXIT_FAILURE, 0, "unexpected accent count: %u", kd->kb_cnt);

	if (kd->kbdiacr[0].diacr != '`' || kd->kbdiacr[0].base != 'a' || kd->kbdiacr[0].result != 0xe0)
		kbd_error(EXIT_FAILURE, 0, "unexpected first accent entry");

	if (kd->kbdiacr[1].diacr != '^' || kd->kbdiacr[1].base != 'o' || kd->kbdiacr[1].result != 0xf4)
		kbd_error(EXIT_FAILURE, 0, "unexpected second accent entry");

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

	dcr.diacr = '`';
	dcr.base = 'a';
	dcr.result = 0x00e0;
	if (lk_add_diacr(ctx, 0, &dcr) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to add first accent");

	dcr.diacr = '^';
	dcr.base = 'o';
	dcr.result = 0x00f4;
	if (lk_add_diacr(ctx, 1, &dcr) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to add second accent");

	ops = ctx->ops;
	ops.ioctl_fn = fake_ioctl;
	lk_set_ops(ctx, &ops);

	if (lk_load_keymap(ctx, TEST_CONSOLE_FD, K_XLATE) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to load compose definitions");

	if (diacr_calls != 1U)
		kbd_error(EXIT_FAILURE, 0, "unexpected compose ioctl count: %u", diacr_calls);

	lk_free(ctx);

	return EXIT_SUCCESS;
}
