#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <linux/kd.h>

#include "kfontP.h"
#include "libcommon.h"

#define TEST_CONSOLE_FD 53

static struct unimapdesc *expected_ud;
static unsigned int clr_call_count;
static unsigned int put_call_count;

static int
fake_ioctl(int fd, unsigned long req, void *arg)
{
	if (fd != TEST_CONSOLE_FD)
		kbd_error(EXIT_FAILURE, 0, "unexpected fd: %d", fd);

	if (req == PIO_UNIMAPCLR) {
		struct unimapinit *ui = arg;

		if (ui->advised_hashsize != 11)
			kbd_error(EXIT_FAILURE, 0, "unexpected advised_hashsize: %u", ui->advised_hashsize);

		if (ui->advised_hashstep != 7)
			kbd_error(EXIT_FAILURE, 0, "unexpected advised_hashstep: %u", ui->advised_hashstep);

		if (ui->advised_hashlevel != clr_call_count + 2U)
			kbd_error(EXIT_FAILURE, 0, "unexpected advised_hashlevel: %u", ui->advised_hashlevel);

		clr_call_count++;
		return 0;
	}

	if (req == PIO_UNIMAP) {
		struct unimapdesc *ud = arg;

		if (ud != expected_ud)
			kbd_error(EXIT_FAILURE, 0, "unexpected unimap descriptor pointer");

		if (ud->entry_ct != 2)
			kbd_error(EXIT_FAILURE, 0, "unexpected entry count: %u", ud->entry_ct);

		if (!ud->entries)
			kbd_error(EXIT_FAILURE, 0, "missing unimap entries");

		if (put_call_count < 2U) {
			put_call_count++;
			errno = ENOMEM;
			return -1;
		}

		put_call_count++;
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
	struct unimapinit ui = {
		.advised_hashsize = 11,
		.advised_hashstep = 7,
		.advised_hashlevel = 2,
	};
	struct unipair entries[2] = {
		{ .unicode = 0x41, .fontpos = 1 },
		{ .unicode = 0x42, .fontpos = 2 },
	};
	struct unimapdesc ud = {
		.entry_ct = 2,
		.entries = entries,
	};

	if (kfont_init("libkfont-test09", &ctx) != 0)
		kbd_error(EXIT_FAILURE, 0, "unable to create kfont context");

	kfont_set_logger(ctx, NULL);

	expected_ud = &ud;

	ops = ctx->ops;
	ops.ioctl_fn = fake_ioctl;
	kfont_set_ops(ctx, &ops);

	if (kfont_put_unicodemap(ctx, TEST_CONSOLE_FD, &ui, &ud) != 0)
		kbd_error(EXIT_FAILURE, 0, "kfont_put_unicodemap failed");

	if (clr_call_count != 3U)
		kbd_error(EXIT_FAILURE, 0, "unexpected PIO_UNIMAPCLR call count: %u", clr_call_count);

	if (put_call_count != 3U)
		kbd_error(EXIT_FAILURE, 0, "unexpected PIO_UNIMAP call count: %u", put_call_count);

	if (ui.advised_hashlevel != 2)
		kbd_error(EXIT_FAILURE, 0, "input unimapinit was modified: %u", ui.advised_hashlevel);

	if (ud.entries != entries)
		kbd_error(EXIT_FAILURE, 0, "input unimapdesc entries pointer changed");

	kfont_free(ctx);

	return EXIT_SUCCESS;
}
