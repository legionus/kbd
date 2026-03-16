#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <linux/kd.h>

#include "kfontP.h"
#include "libcommon.h"

static unsigned char *expected_buf;
static unsigned int expected_count;

static int
fake_ioctl_put_font_retry(int fd, unsigned long req, void *arg)
{
	if (fd != 37)
		kbd_error(EXIT_FAILURE, 0, "unexpected fd: %d", fd);

	if (req == KDGETMODE) {
		*(unsigned int *) arg = KD_TEXT;
		return 0;
	}

	if (req == KDFONTOP) {
		struct console_font_op *op = arg;
		size_t used = 32U * expected_count;
		size_t i;

		if (op->op != KD_FONT_OP_SET)
			kbd_error(EXIT_FAILURE, 0, "unexpected font op: %u", op->op);

		if (op->charcount == expected_count) {
			if (op->data != expected_buf)
				kbd_error(EXIT_FAILURE, 0, "unexpected first-attempt buffer");
			errno = EINVAL;
			return -1;
		}

		if (op->charcount != 256)
			kbd_error(EXIT_FAILURE, 0, "unexpected retried charcount: %u", op->charcount);

		if (op->data == expected_buf)
			kbd_error(EXIT_FAILURE, 0, "retry did not copy the font buffer");

		for (i = 0; i < used; i++) {
			if (op->data[i] != expected_buf[i])
				kbd_error(EXIT_FAILURE, 0, "unexpected copied byte at %zu", i);
		}

		return 0;
	}

	return -1;
}

static int
fake_ioctl_put_font_fail(int fd, unsigned long req, void *arg)
{
	if (fd != 41)
		kbd_error(EXIT_FAILURE, 0, "unexpected fd: %d", fd);

	if (req == KDGETMODE) {
		*(unsigned int *) arg = KD_TEXT;
		return 0;
	}

	if (req == KDFONTOP) {
		struct console_font_op *op = arg;

		if (op->op != KD_FONT_OP_SET)
			kbd_error(EXIT_FAILURE, 0, "unexpected font op: %u", op->op);

		if (op->data != expected_buf)
			kbd_error(EXIT_FAILURE, 0, "unexpected failure-path buffer");

		errno = ENOSYS;
		return -1;
	}

	return -1;
}

static int
fake_ioctl_put_font_tall(int fd, unsigned long req, void *arg)
{
	if (fd != 43)
		kbd_error(EXIT_FAILURE, 0, "unexpected fd: %d", fd);

	if (req == KDGETMODE) {
		*(unsigned int *) arg = KD_TEXT;
		return 0;
	}

	if (req == KDFONTOP) {
		struct console_font_op *op = arg;

		if (op->op != KD_FONT_OP_SET_TALL)
			kbd_error(EXIT_FAILURE, 0, "unexpected font op: %u", op->op);

		if (op->data != expected_buf)
			kbd_error(EXIT_FAILURE, 0, "unexpected tall-font buffer");

		return 0;
	}

	return -1;
}

static void
test_put_font_retry_after_einval(void)
{
	struct kfont_context *ctx;
	struct kfont_ops ops;
	unsigned char buf[17U * 32U];
	size_t i;

	if (kfont_init("libkfont-test05", &ctx) != 0)
		kbd_error(EXIT_FAILURE, 0, "unable to create kfont context");

	for (i = 0; i < sizeof(buf); i++)
		buf[i] = (unsigned char) (i & 0xff);

	expected_buf = buf;
	expected_count = 17U;
	kfont_set_logger(ctx, NULL);
	ops = ctx->ops;
	ops.ioctl_fn = fake_ioctl_put_font_retry;
	kfont_set_ops(ctx, &ops);

	if (kfont_put_font(ctx, 37, buf, expected_count, 8, 16, 32) != 0)
		kbd_error(EXIT_FAILURE, 0, "kfont_put_font failed");

	kfont_free(ctx);
}

static void
test_put_font_failure(void)
{
	struct kfont_context *ctx;
	struct kfont_ops ops;
	unsigned char buf[32U * 32U];
	size_t i;

	if (kfont_init("libkfont-test05", &ctx) != 0)
		kbd_error(EXIT_FAILURE, 0, "unable to create kfont context");

	for (i = 0; i < sizeof(buf); i++)
		buf[i] = (unsigned char) ((i + 1) & 0xff);

	expected_buf = buf;
	kfont_set_logger(ctx, NULL);
	ops = ctx->ops;
	ops.ioctl_fn = fake_ioctl_put_font_fail;
	kfont_set_ops(ctx, &ops);

	if (kfont_put_font(ctx, 41, buf, 32, 8, 16, 32) == 0)
		kbd_error(EXIT_FAILURE, 0, "kfont_put_font unexpectedly succeeded");

	kfont_free(ctx);
}

static void
test_put_tall_font(void)
{
	struct kfont_context *ctx;
	struct kfont_ops ops;
	unsigned char buf[8U * 48U * 5U];
	size_t i;

	if (kfont_init("libkfont-test05", &ctx) != 0)
		kbd_error(EXIT_FAILURE, 0, "unable to create kfont context");

	for (i = 0; i < sizeof(buf); i++)
		buf[i] = (unsigned char) ((i * 3U) & 0xff);

	expected_buf = buf;
	kfont_set_logger(ctx, NULL);
	ops = ctx->ops;
	ops.ioctl_fn = fake_ioctl_put_font_tall;
	kfont_set_ops(ctx, &ops);

	if (kfont_put_font(ctx, 43, buf, 8, 40, 24, 48) != 0)
		kbd_error(EXIT_FAILURE, 0, "kfont_put_font failed for tall font");

	kfont_free(ctx);
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	test_put_font_retry_after_einval();
	test_put_font_failure();
	test_put_tall_font();
	return EXIT_SUCCESS;
}
