#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <linux/kd.h>

#include "kfontP.h"
#include "libcommon.h"

#define TEST_CONSOLE_FD 61

static unsigned int clr_call_count;
static unsigned int put_call_count;

static int
fake_ioctl(int fd, unsigned long req, void *arg)
{
	if (fd != TEST_CONSOLE_FD)
		kbd_error(EXIT_FAILURE, 0, "unexpected fd: %d", fd);

	if (req == PIO_UNIMAPCLR) {
		struct unimapinit *ui = arg;

		if (ui->advised_hashsize != 13)
			kbd_error(EXIT_FAILURE, 0, "unexpected advised_hashsize: %u", ui->advised_hashsize);

		if (ui->advised_hashstep != 9)
			kbd_error(EXIT_FAILURE, 0, "unexpected advised_hashstep: %u", ui->advised_hashstep);

		if (ui->advised_hashlevel != 4)
			kbd_error(EXIT_FAILURE, 0, "unexpected advised_hashlevel: %u", ui->advised_hashlevel);

		clr_call_count++;
		errno = EINVAL;
		return -1;
	}

	if (req == PIO_UNIMAP) {
		put_call_count++;
		kbd_error(EXIT_FAILURE, 0, "PIO_UNIMAP should not be called after PIO_UNIMAPCLR failure");
	}

	kbd_error(EXIT_FAILURE, 0, "unexpected request: %lu", req);
	return -1;
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct kfont_context *ctx;
	struct kfont_ops ops;
	struct unimapinit ui = {
		.advised_hashsize = 13,
		.advised_hashstep = 9,
		.advised_hashlevel = 4,
	};
	struct unipair entries[1] = {
		{ .unicode = 0x41, .fontpos = 1 },
	};
	struct unimapdesc ud = {
		.entry_ct = 1,
		.entries = entries,
	};

	if (kfont_init("libkfont-test11", &ctx) != 0)
		kbd_error(EXIT_FAILURE, 0, "unable to create kfont context");

	kfont_set_logger(ctx, NULL);

	ops = ctx->ops;
	ops.ioctl_fn = fake_ioctl;
	kfont_set_ops(ctx, &ops);

	if (kfont_put_unicodemap(ctx, TEST_CONSOLE_FD, &ui, &ud) == 0)
		kbd_error(EXIT_FAILURE, 0, "kfont_put_unicodemap unexpectedly succeeded");

	if (clr_call_count != 1U)
		kbd_error(EXIT_FAILURE, 0, "unexpected PIO_UNIMAPCLR call count: %u", clr_call_count);

	if (put_call_count != 0U)
		kbd_error(EXIT_FAILURE, 0, "unexpected PIO_UNIMAP call count: %u", put_call_count);

	if (ui.advised_hashlevel != 4)
		kbd_error(EXIT_FAILURE, 0, "input unimapinit was modified: %u", ui.advised_hashlevel);

	if (ud.entries != entries)
		kbd_error(EXIT_FAILURE, 0, "input unimapdesc entries pointer changed");

	kfont_free(ctx);

	return EXIT_SUCCESS;
}
