#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <linux/kd.h>

#include <kfont.h>
#include "kfontP.h"
#include "libcommon.h"

#define TEST_CONSOLE_FD 127

static unsigned int current_mode = KD_TEXT;

static int
fake_ioctl(int fd, unsigned long req, void *arg)
{
	if (fd != TEST_CONSOLE_FD)
		kbd_error(EXIT_FAILURE, 0, "unexpected fd: %d", fd);

	if (req == KDGETMODE) {
		*(unsigned int *)arg = current_mode;
		return 0;
	}

	if (req == KDFONTOP) {
		struct console_font_op *cfo = arg;

		if (cfo->op == KD_FONT_OP_GET) {
			cfo->charcount = 512;
			cfo->width = 8;
			cfo->height = 16;

			if (cfo->data)
				memset(cfo->data, 0, cfo->charcount * 32U);

			return 0;
		}

		if (cfo->op == KD_FONT_OP_SET_DEFAULT)
			return 0;
	}

	return -1;
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct kfont_context *ctx;
	struct kfont_ops ops;
	unsigned char fontbuf[32];

	if (kfont_init("libkfont-test25", &ctx) != 0)
		kbd_error(EXIT_FAILURE, 0, "unable to create kfont context");

	kfont_set_logger(ctx, NULL);

	ops = ctx->ops;
	ops.ioctl_fn = fake_ioctl;
	kfont_set_ops(ctx, &ops);

	if (kfont_get_fontsize(ctx, TEST_CONSOLE_FD) != 512)
		kbd_error(EXIT_FAILURE, 0, "unexpected font size");

	if (!kfont_is_font_console(ctx, TEST_CONSOLE_FD))
		kbd_error(EXIT_FAILURE, 0, "text console was not detected");

	if (kfont_restore_font(ctx, TEST_CONSOLE_FD) != 0)
		kbd_error(EXIT_FAILURE, 0, "unable to restore default font");

	memset(fontbuf, 0, sizeof(fontbuf));
	fontbuf[2] = 1;
	if (font_charheight(fontbuf, 1, 8) != 3)
		kbd_error(EXIT_FAILURE, 0, "unexpected font height");

	current_mode = KD_GRAPHICS;

	if (kfont_is_font_console(ctx, TEST_CONSOLE_FD))
		kbd_error(EXIT_FAILURE, 0, "graphics console unexpectedly accepted");

	if (kfont_restore_font(ctx, TEST_CONSOLE_FD) == 0)
		kbd_error(EXIT_FAILURE, 0, "restoring font unexpectedly succeeded in graphics mode");

	kfont_free(ctx);

	return EXIT_SUCCESS;
}
