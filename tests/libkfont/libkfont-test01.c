#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <linux/kd.h>

#include "kfontP.h"
#include "libcommon.h"

static int
fake_ioctl_get_unicodemap(int fd, unsigned long req, void *arg)
{
	struct unimapdesc *ud = arg;

	if (fd != 17)
		kbd_error(EXIT_FAILURE, 0, "unexpected fd: %d", fd);

	if (req != GIO_UNIMAP)
		kbd_error(EXIT_FAILURE, 0, "unexpected request: %lu", req);

	if (!ud->entries) {
		ud->entry_ct = 2;
		errno = ENOMEM;
		return -1;
	}

	ud->entry_ct = 2;
	ud->entries[0].unicode = 0x41;
	ud->entries[0].fontpos = 1;
	ud->entries[1].unicode = 0x42;
	ud->entries[1].fontpos = 2;
	return 0;
}

static int
fake_ioctl_get_font_regular(int fd, unsigned long req, void *arg)
{
	if (fd != 23)
		kbd_error(EXIT_FAILURE, 0, "unexpected fd: %d", fd);

	if (req == KDGETMODE) {
		*(unsigned int *) arg = KD_TEXT;
		return 0;
	}

	if (req == KDFONTOP) {
		struct console_font_op *op = arg;
		size_t glyph_size = 32;
		size_t font_size = glyph_size * 3;

		if (op->op != KD_FONT_OP_GET)
			kbd_error(EXIT_FAILURE, 0, "unexpected font op: %u", op->op);

		if (!op->data) {
			op->width = 8;
			op->height = 16;
			op->charcount = 3;
			return 0;
		}

		op->width = 8;
		op->height = 16;
		op->charcount = 3;
		memset(op->data, 0x5a, font_size);
		return 0;
	}

	kbd_error(EXIT_FAILURE, 0, "unexpected request: %lu", req);
	return -1;
}

static int
fake_ioctl_get_font_tall(int fd, unsigned long req, void *arg)
{
	if (fd != 29)
		kbd_error(EXIT_FAILURE, 0, "unexpected fd: %d", fd);

	if (req == KDGETMODE) {
		*(unsigned int *) arg = KD_TEXT;
		return 0;
	}

	if (req == KDFONTOP) {
		struct console_font_op *op = arg;
		size_t glyph_size = 40;
		size_t font_size = glyph_size * 2;

		if (op->op == KD_FONT_OP_GET) {
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

		op->width = 8;
		op->height = 40;
		op->charcount = 2;
		memset(op->data, 0x6b, font_size);
		return 0;
	}

	kbd_error(EXIT_FAILURE, 0, "unexpected request: %lu", req);
	return -1;
}

static int
fake_ioctl_get_font_fail(int fd, unsigned long req, void *arg)
{
	if (fd != 31)
		kbd_error(EXIT_FAILURE, 0, "unexpected fd: %d", fd);

	if (req == KDGETMODE) {
		*(unsigned int *) arg = KD_TEXT;
		return 0;
	}

	if (req == KDFONTOP) {
		struct console_font_op *op = arg;

		if (op->op != KD_FONT_OP_GET)
			kbd_error(EXIT_FAILURE, 0, "unexpected font op: %u", op->op);

		if (!op->data) {
			op->width = 8;
			op->height = 16;
			op->charcount = 3;
			return 0;
		}

		errno = EIO;
		return -1;
	}

	kbd_error(EXIT_FAILURE, 0, "unexpected request: %lu", req);
	return -1;
}

static void
test_get_unicodemap_retry(void)
{
	struct kfont_context *ctx;
	struct kfont_ops ops;
	struct unimapdesc ud;

	if (kfont_init("libkfont-test01", &ctx) != 0)
		kbd_error(EXIT_FAILURE, 0, "unable to create kfont context");

	kfont_set_logger(ctx, NULL);
	ops = ctx->ops;
	ops.ioctl_fn = fake_ioctl_get_unicodemap;
	kfont_set_ops(ctx, &ops);

	memset(&ud, 0, sizeof(ud));
	if (kfont_get_unicodemap(ctx, 17, &ud) != 0)
		kbd_error(EXIT_FAILURE, 0, "kfont_get_unicodemap failed");

	if (ud.entry_ct != 2 || !ud.entries)
		kbd_error(EXIT_FAILURE, 0, "unexpected unicode map allocation result");

	if (ud.entries[0].unicode != 0x41 || ud.entries[1].fontpos != 2)
		kbd_error(EXIT_FAILURE, 0, "unexpected unicode map contents");

	free(ud.entries);
	kfont_free(ctx);
}

static void
test_get_font_regular(void)
{
	struct kfont_context *ctx;
	struct kfont_ops ops;
	unsigned char buf[4 * 32];
	unsigned int count = 4, width = 0, height = 0, vpitch = 0;
	size_t i;

	if (kfont_init("libkfont-test01", &ctx) != 0)
		kbd_error(EXIT_FAILURE, 0, "unable to create kfont context");

	kfont_set_logger(ctx, NULL);
	ops = ctx->ops;
	ops.ioctl_fn = fake_ioctl_get_font_regular;
	kfont_set_ops(ctx, &ops);

	memset(buf, 0, sizeof(buf));
	if (kfont_get_font(ctx, 23, buf, &count, &width, &height, &vpitch) != 0)
		kbd_error(EXIT_FAILURE, 0, "kfont_get_font failed");

	if (count != 3 || width != 8 || height != 16 || vpitch != 32)
		kbd_error(EXIT_FAILURE, 0, "unexpected regular font geometry");

	for (i = 0; i < 3U * 32U; i++) {
		if (buf[i] != 0x5a)
			kbd_error(EXIT_FAILURE, 0, "unexpected regular font byte at %zu", i);
	}

	kfont_free(ctx);
}

static void
test_get_font_tall_retry(void)
{
	struct kfont_context *ctx;
	struct kfont_ops ops;
	unsigned char buf[4 * 128];
	unsigned int count = 4, width = 0, height = 0, vpitch = 0;
	size_t i;

	if (kfont_init("libkfont-test01", &ctx) != 0)
		kbd_error(EXIT_FAILURE, 0, "unable to create kfont context");

	kfont_set_logger(ctx, NULL);
	ops = ctx->ops;
	ops.ioctl_fn = fake_ioctl_get_font_tall;
	kfont_set_ops(ctx, &ops);

	memset(buf, 0, sizeof(buf));
	if (kfont_get_font(ctx, 29, buf, &count, &width, &height, &vpitch) != 0)
		kbd_error(EXIT_FAILURE, 0, "kfont_get_font failed");

	if (count != 2 || width != 8 || height != 40 || vpitch != 40)
		kbd_error(EXIT_FAILURE, 0, "unexpected tall font geometry");

	for (i = 0; i < 2U * 40U; i++) {
		if (buf[i] != 0x6b)
			kbd_error(EXIT_FAILURE, 0, "unexpected tall font byte at %zu", i);
	}

	kfont_free(ctx);
}

static void
test_get_font_failure_preserves_buffer(void)
{
	struct kfont_context *ctx;
	struct kfont_ops ops;
	unsigned char buf[4 * 32];
	unsigned int count = 4, width = 0, height = 0, vpitch = 0;
	size_t i;

	if (kfont_init("libkfont-test01", &ctx) != 0)
		kbd_error(EXIT_FAILURE, 0, "unable to create kfont context");

	kfont_set_logger(ctx, NULL);
	ops = ctx->ops;
	ops.ioctl_fn = fake_ioctl_get_font_fail;
	kfont_set_ops(ctx, &ops);

	memset(buf, 0x7c, sizeof(buf));
	if (kfont_get_font(ctx, 31, buf, &count, &width, &height, &vpitch) == 0)
		kbd_error(EXIT_FAILURE, 0, "kfont_get_font unexpectedly succeeded");

	for (i = 0; i < sizeof(buf); i++) {
		if (buf[i] != 0x7c)
			kbd_error(EXIT_FAILURE, 0, "unexpected modified byte at %zu", i);
	}

	kfont_free(ctx);
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	test_get_unicodemap_retry();
	test_get_font_regular();
	test_get_font_tall_retry();
	test_get_font_failure_preserves_buffer();
	return EXIT_SUCCESS;
}
