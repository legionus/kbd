#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <linux/kd.h>

#include <keymap.h>
#include "contextP.h"
#include "libcommon.h"

#define TEST_CONSOLE_FD 71

#ifdef KDGKBDIACRUC
#define TEST_KDGKBDIACR_REQ KDGKBDIACRUC
#define TEST_KBDIACRS struct kbdiacrsuc
#define TEST_KBDIACR_FIELD kbdiacruc
#else
#define TEST_KDGKBDIACR_REQ KDGKBDIACR
#define TEST_KBDIACRS struct kbdiacrs
#define TEST_KBDIACR_FIELD kbdiacr
#endif

static int
fake_ioctl(int fd, unsigned long req, uintptr_t arg)
{
	TEST_KBDIACRS *kd = (TEST_KBDIACRS *)arg;

	if (fd != TEST_CONSOLE_FD)
		kbd_error(EXIT_FAILURE, 0, "unexpected fd: %d", fd);

	if (req != TEST_KDGKBDIACR_REQ)
		kbd_error(EXIT_FAILURE, 0, "unexpected request: %lu", req);

	kd->kb_cnt = 2;
	kd->TEST_KBDIACR_FIELD[0].diacr = '`';
	kd->TEST_KBDIACR_FIELD[0].base = 'a';
	kd->TEST_KBDIACR_FIELD[0].result = 0x00e0;
	kd->TEST_KBDIACR_FIELD[1].diacr = '^';
	kd->TEST_KBDIACR_FIELD[1].base = 'o';
	kd->TEST_KBDIACR_FIELD[1].result = 0x00f4;

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

	ops = ctx->ops;
	ops.ioctl_fn = fake_ioctl;
	lk_set_ops(ctx, &ops);

	if (lk_kernel_diacrs(ctx, TEST_CONSOLE_FD) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to load accent table from kernel");

	if (!lk_diacr_exists(ctx, 0))
		kbd_error(EXIT_FAILURE, 0, "Accent 0 was not loaded");

	if (!lk_diacr_exists(ctx, 1))
		kbd_error(EXIT_FAILURE, 0, "Accent 1 was not loaded");

	if (lk_get_diacr(ctx, 0, &dcr) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to read accent 0");

	if (dcr.diacr != '`' || dcr.base != 'a' || dcr.result != 0x00e0)
		kbd_error(EXIT_FAILURE, 0, "Unexpected accent 0 contents");

	if (lk_get_diacr(ctx, 1, &dcr) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to read accent 1");

	if (dcr.diacr != '^' || dcr.base != 'o' || dcr.result != 0x00f4)
		kbd_error(EXIT_FAILURE, 0, "Unexpected accent 1 contents");

	lk_free(ctx);

	return EXIT_SUCCESS;
}
