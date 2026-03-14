#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <linux/kd.h>

#include "kfontP.h"
#include "libcommon.h"

#define TEST_CONSOLE_FD 47

static unsigned short *expected_map;

static int
fake_ioctl(int fd, unsigned long req, void *arg)
{
	unsigned short *map = arg;
	size_t i;

	if (fd != TEST_CONSOLE_FD)
		kbd_error(EXIT_FAILURE, 0, "unexpected fd: %d", fd);

	if (req != PIO_UNISCRNMAP)
		kbd_error(EXIT_FAILURE, 0, "unexpected request: %lu", req);

	if (map == expected_map)
		kbd_error(EXIT_FAILURE, 0, "ioctl received original map buffer");

	for (i = 0; i < E_TABSZ; i++) {
		if (map[i] != expected_map[i])
			kbd_error(EXIT_FAILURE, 0, "unexpected map entry at %zu: %#x", i, map[i]);
	}

	for (i = 0; i < E_TABSZ; i++)
		map[i] = 0;

	return 0;
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct kfont_context *ctx;
	struct kfont_ops ops;
	unsigned short map[E_TABSZ];
	size_t i;

	if (kfont_init("libkfont-test08", &ctx) != 0)
		kbd_error(EXIT_FAILURE, 0, "unable to create kfont context");

	kfont_set_logger(ctx, NULL);

	for (i = 0; i < E_TABSZ; i++)
		map[i] = (unsigned short)((i * 17U) & 0xffffU);

	expected_map = map;

	ops = ctx->ops;
	ops.ioctl_fn = fake_ioctl;
	kfont_set_ops(ctx, &ops);

	if (kfont_put_uniscrnmap(ctx, TEST_CONSOLE_FD, map) != 0)
		kbd_error(EXIT_FAILURE, 0, "kfont_put_uniscrnmap failed");

	for (i = 0; i < E_TABSZ; i++) {
		unsigned short expected = (unsigned short)((i * 17U) & 0xffffU);

		if (map[i] != expected)
			kbd_error(EXIT_FAILURE, 0, "source map changed at %zu: %#x", i, map[i]);
	}

	kfont_free(ctx);

	return EXIT_SUCCESS;
}
