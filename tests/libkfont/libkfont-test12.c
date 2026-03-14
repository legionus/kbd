#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <linux/kd.h>

#include <kfont.h>
#include "kfontP.h"
#include "libcommon.h"

#define TEST_CONSOLE_FD 67

static unsigned int get_call_count;

static int
fake_ioctl(int fd, unsigned long req, void *arg)
{
	unsigned short *map = arg;
	size_t i;

	if (fd != TEST_CONSOLE_FD)
		kbd_error(EXIT_FAILURE, 0, "unexpected fd: %d", fd);

	if (req == GIO_UNISCRNMAP) {
		get_call_count++;

		for (i = 0; i < E_TABSZ; i++)
			map[i] = (unsigned short)((0xf000U + i) & 0xffffU);

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
	unsigned short get_map[E_TABSZ];
	size_t i;

	if (kfont_init("libkfont-test12", &ctx) != 0)
		kbd_error(EXIT_FAILURE, 0, "unable to create kfont context");

	kfont_set_logger(ctx, NULL);

	ops = ctx->ops;
	ops.ioctl_fn = fake_ioctl;
	kfont_set_ops(ctx, &ops);

	memset(get_map, 0, sizeof(get_map));

	if (kfont_get_uniscrnmap(ctx, TEST_CONSOLE_FD, get_map) != 0)
		kbd_error(EXIT_FAILURE, 0, "kfont_get_uniscrnmap failed");

	for (i = 0; i < E_TABSZ; i++) {
		unsigned short expected = (unsigned short)((0xf000U + i) & 0xffffU);

		if (get_map[i] != expected)
			kbd_error(EXIT_FAILURE, 0, "unexpected fetched map entry at %zu: %#x", i, get_map[i]);
	}

	if (get_call_count != 1U)
		kbd_error(EXIT_FAILURE, 0, "unexpected get_uniscrnmap ioctl count: %u", get_call_count);

	kfont_free(ctx);

	return EXIT_SUCCESS;
}
