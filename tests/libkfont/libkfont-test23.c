#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <sysexits.h>

#include <linux/kd.h>

#include "kfontP.h"
#include "libcommon.h"

#define TEST_CONSOLE_FD 89

static unsigned int gio_scrnmap_calls;
static unsigned int gio_uniscrnmap_calls;

static int
fake_ioctl(int fd, unsigned long req, void *arg)
{
	unsigned int i;

	if (fd != TEST_CONSOLE_FD)
		kbd_error(EXIT_FAILURE, 0, "unexpected fd: %d", fd);

	if (req == GIO_SCRNMAP) {
		unsigned char *map = arg;

		gio_scrnmap_calls++;

		for (i = 0; i < E_TABSZ; i++)
			map[i] = (unsigned char)i;

		return 0;
	}

	if (req == GIO_UNISCRNMAP) {
		unsigned short *map = arg;

		gio_uniscrnmap_calls++;

		for (i = 0; i < E_TABSZ; i++)
			map[i] = (unsigned short)(0xf000U + i);

		return 0;
	}

	kbd_error(EXIT_FAILURE, 0, "unexpected request: %lu", req);
	return -1;
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct kfont_context *ctx;
	struct kfont_ops ops;
	int ret;

	if (kfont_init("libkfont-test23", &ctx) != 0)
		kbd_error(EXIT_FAILURE, 0, "unable to create kfont context");

	kfont_set_logger(ctx, NULL);

	ops = ctx->ops;
	ops.ioctl_fn = fake_ioctl;
	kfont_set_ops(ctx, &ops);

	/* Opening an output file in a missing directory should fail immediately. */
	ret = kfont_save_consolemap(ctx, TEST_CONSOLE_FD, "/tmp/no-such-dir/consolemap.out");
	if (ret != -EX_DATAERR)
		kbd_error(EXIT_FAILURE, 0, "unexpected fopen failure result: %d", ret);

	if (gio_scrnmap_calls != 0U || gio_uniscrnmap_calls != 0U)
		kbd_error(EXIT_FAILURE, 0, "ioctl should not run after fopen failure");

	kfont_free(ctx);

	return EXIT_SUCCESS;
}
