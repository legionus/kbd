#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <linux/kd.h>

#include "kfontP.h"
#include "libcommon.h"

#define TEST_FONT_COUNT 8U
#define TEST_FONT_WIDTH 40U
#define TEST_FONT_HEIGHT 24U
#define TEST_FONT_VPITCH 48U
#define TEST_CONSOLE_FD 43

static unsigned char *expected_buf;

static int
fake_ioctl(int fd, unsigned long req, void *arg)
{
	if (fd != TEST_CONSOLE_FD)
		kbd_error(EXIT_FAILURE, 0, "unexpected fd: %d", fd);

	if (req == KDGETMODE) {
		unsigned int *mode = arg;

		*mode = KD_TEXT;
		return 0;
	}

	if (req == KDFONTOP) {
		struct console_font_op *op = arg;

		if (op->op != KD_FONT_OP_SET_TALL)
			kbd_error(EXIT_FAILURE, 0, "unexpected font op: %u", op->op);

		if (op->width != TEST_FONT_WIDTH)
			kbd_error(EXIT_FAILURE, 0, "unexpected width: %u", op->width);

		if (op->height != TEST_FONT_HEIGHT)
			kbd_error(EXIT_FAILURE, 0, "unexpected height: %u", op->height);

		if (op->charcount != TEST_FONT_COUNT)
			kbd_error(EXIT_FAILURE, 0, "unexpected charcount: %u", op->charcount);

		if (op->data != expected_buf)
			kbd_error(EXIT_FAILURE, 0, "unexpected buffer pointer");

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
	unsigned int bytewidth = (TEST_FONT_WIDTH + 7U) / 8U;
	unsigned char buf[TEST_FONT_COUNT * TEST_FONT_VPITCH * ((TEST_FONT_WIDTH + 7U) / 8U)];
	size_t i;

	if (kfont_init("libkfont-test07", &ctx) != 0)
		kbd_error(EXIT_FAILURE, 0, "unable to create kfont context");

	kfont_set_logger(ctx, NULL);

	for (i = 0; i < sizeof(buf); i++)
		buf[i] = (unsigned char)((i * 3U) & 0xff);

	if (bytewidth != 5U)
		kbd_error(EXIT_FAILURE, 0, "unexpected bytewidth: %u", bytewidth);

	expected_buf = buf;

	ops = ctx->ops;
	ops.ioctl_fn = fake_ioctl;
	kfont_set_ops(ctx, &ops);

	if (kfont_put_font(ctx, TEST_CONSOLE_FD, buf, TEST_FONT_COUNT,
			TEST_FONT_WIDTH, TEST_FONT_HEIGHT, TEST_FONT_VPITCH) != 0)
		kbd_error(EXIT_FAILURE, 0, "kfont_put_font failed");

	for (i = 0; i < sizeof(buf); i++) {
		if (buf[i] != (unsigned char)((i * 3U) & 0xff))
			kbd_error(EXIT_FAILURE, 0, "source buffer changed at %zu: %#x", i, buf[i]);
	}

	kfont_free(ctx);

	return EXIT_SUCCESS;
}
