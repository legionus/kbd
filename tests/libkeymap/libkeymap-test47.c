#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <linux/kd.h>

#include <keymap.h>
#include "contextP.h"
#include "libcommon.h"

#define TEST_CONSOLE_FD 109

static int
fake_ioctl(int fd, unsigned long req, uintptr_t arg)
{
	struct kbentry *ke = (struct kbentry *)arg;

	if (fd != TEST_CONSOLE_FD)
		kbd_error(EXIT_FAILURE, 0, "unexpected fd: %d", fd);

	if (req == KDGKBENT)
		return 0;

	if (req != KDSKBENT)
		kbd_error(EXIT_FAILURE, 0, "unexpected request: %lu", req);

	if (KTYP(ke->kb_value) == 0 && KVAL(ke->kb_value) < 4)
		return 0;

	errno = EINVAL;
	return -1;
}

static void
expect_contains(const char *buf, const char *needle)
{
	if (!strstr(buf, needle))
		kbd_error(EXIT_FAILURE, 0, "missing output fragment: %s", needle);
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct lk_ctx *ctx;
	char *buf = NULL;
	size_t size = 0;
	FILE *fp;

	ctx = lk_init();
	if (!ctx)
		kbd_error(EXIT_FAILURE, 0, "Unable to initialize structure by valid pointer");

	lk_set_log_fn(ctx, NULL, NULL);

	if (lk_add_key(ctx, 0, 0, K_ALLOCATED) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to mark allocated keymap");

	{
		struct lk_ops ops = ctx->ops;
		ops.ioctl_fn = fake_ioctl;
		lk_set_ops(ctx, &ops);
	}

	fp = open_memstream(&buf, &size);
	if (!fp)
		kbd_error(EXIT_FAILURE, 0, "Unable to allocate memory stream");

	lk_dump_summary(ctx, fp, TEST_CONSOLE_FD);
	fclose(fp);

	expect_contains(buf, "number of keymaps in actual use:");
	expect_contains(buf, "of which 1 dynamically allocated");
	expect_contains(buf, "0x0000 - 0x0003");

	free(buf);
	lk_free(ctx);

	return EXIT_SUCCESS;
}
