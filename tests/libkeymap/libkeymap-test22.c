#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <linux/kd.h>

#include <keymap.h>
#include "contextP.h"
#include "libcommon.h"

#define TEST_CONSOLE_FD 73

static int
fake_ioctl(int fd, unsigned long req, uintptr_t arg)
{
	struct kbsentry *kbs = (struct kbsentry *)arg;

	if (fd != TEST_CONSOLE_FD)
		kbd_error(EXIT_FAILURE, 0, "unexpected fd: %d", fd);

	if (req != KDGKBSENT)
		kbd_error(EXIT_FAILURE, 0, "unexpected request: %lu", req);

	memset(kbs->kb_string, 0, sizeof(kbs->kb_string));

	switch (kbs->kb_func) {
		case 0:
			strcpy((char *)kbs->kb_string, "first");
			break;
		case 1:
			strcpy((char *)kbs->kb_string, "");
			break;
		case 2:
			strcpy((char *)kbs->kb_string, "third");
			break;
		default:
			break;
	}

	return 0;
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct lk_ctx *ctx;
	struct lk_ops ops;
	struct kbsentry kbs;

	ctx = lk_init();
	if (!ctx)
		kbd_error(EXIT_FAILURE, 0, "Unable to initialize structure by valid pointer");

	lk_set_log_fn(ctx, NULL, NULL);

	ops = ctx->ops;
	ops.ioctl_fn = fake_ioctl;
	lk_set_ops(ctx, &ops);

	if (lk_kernel_funcs(ctx, TEST_CONSOLE_FD) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to load function strings from kernel");

	kbs.kb_func = 0;
	kbs.kb_string[0] = 0;
	if (lk_get_func(ctx, &kbs) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to read function 0");

	if (strcmp((char *)kbs.kb_string, "first") != 0)
		kbd_error(EXIT_FAILURE, 0, "Unexpected function 0 string");

	kbs.kb_func = 1;
	kbs.kb_string[0] = 0;
	if (lk_get_func(ctx, &kbs) == 0)
		kbd_error(EXIT_FAILURE, 0, "Unexpected function 1 string");

	kbs.kb_func = 2;
	kbs.kb_string[0] = 0;
	if (lk_get_func(ctx, &kbs) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to read function 2");

	if (strcmp((char *)kbs.kb_string, "third") != 0)
		kbd_error(EXIT_FAILURE, 0, "Unexpected function 2 string");

	lk_free(ctx);

	return EXIT_SUCCESS;
}
