#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <linux/kd.h>

#include "kfontP.h"
#include "libcommon.h"

static int
fake_ioctl(int fd, unsigned long req, void *arg)
{
	if (fd != 29)
		kbd_error(EXIT_FAILURE, 0, "unexpected fd: %d", fd);

	if (req == KDGETMODE) {
		unsigned int *mode = arg;

		*mode = KD_TEXT;
		return 0;
	}

	if (req == KDFONTOP) {
		struct console_font_op *op = arg;
		size_t glyph_size = 40;
		size_t font_size = glyph_size * 2;

		if (op->op == KD_FONT_OP_GET) {
			if (op->data)
				kbd_error(EXIT_FAILURE, 0, "unexpected data for KD_FONT_OP_GET");

			errno = ENOSPC;
			return -1;
		}

		if (op->op != KD_FONT_OP_GET_TALL)
			kbd_error(EXIT_FAILURE, 0, "unexpected font op: %u", op->op);

		if (!op->data) {
			op->width = 8;
			op->height = 40;
			op->charcount = 2;
			return 0;
		}

		if (op->charcount < 2)
			kbd_error(EXIT_FAILURE, 0, "buffer too small: %u", op->charcount);

		op->width = 8;
		op->height = 40;
		op->charcount = 2;

		memset(op->data, 0x6b, font_size);
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
	unsigned char buf[4 * 128];
	unsigned int count = 4;
	unsigned int width = 0;
	unsigned int height = 0;
	unsigned int vpitch = 0;
	size_t i;

	if (kfont_init("libkfont-test03", &ctx) != 0)
		kbd_error(EXIT_FAILURE, 0, "unable to create kfont context");

	kfont_set_logger(ctx, NULL);

	ops = ctx->ops;
	ops.ioctl_fn = fake_ioctl;
	kfont_set_ops(ctx, &ops);

	memset(buf, 0, sizeof(buf));

	if (kfont_get_font(ctx, 29, buf, &count, &width, &height, &vpitch) != 0)
		kbd_error(EXIT_FAILURE, 0, "kfont_get_font failed");

	if (count != 2)
		kbd_error(EXIT_FAILURE, 0, "unexpected charcount: %u", count);

	if (width != 8)
		kbd_error(EXIT_FAILURE, 0, "unexpected width: %u", width);

	if (height != 40)
		kbd_error(EXIT_FAILURE, 0, "unexpected height: %u", height);

	if (vpitch != 40)
		kbd_error(EXIT_FAILURE, 0, "unexpected vpitch: %u", vpitch);

	for (i = 0; i < 2U * 40U; i++) {
		if (buf[i] != 0x6b)
			kbd_error(EXIT_FAILURE, 0, "unexpected font byte at %zu: %#x", i, buf[i]);
	}

	for (; i < sizeof(buf); i++) {
		if (buf[i] != 0)
			kbd_error(EXIT_FAILURE, 0, "unexpected trailing font byte at %zu: %#x", i, buf[i]);
	}

	kfont_free(ctx);

	return EXIT_SUCCESS;
}
