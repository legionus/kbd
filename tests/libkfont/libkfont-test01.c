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

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct kfont_context *ctx;
	struct kfont_ops ops;
	struct unimapdesc ud;

	if (kfont_init("libkfont-test01", &ctx) != 0)
		kbd_error(EXIT_FAILURE, 0, "unable to create kfont context");

	kfont_set_logger(ctx, NULL);

	ops = ctx->ops;
	ops.ioctl_fn = fake_ioctl;
	kfont_set_ops(ctx, &ops);

	memset(&ud, 0, sizeof(ud));

	if (kfont_get_unicodemap(ctx, 17, &ud) != 0)
		kbd_error(EXIT_FAILURE, 0, "kfont_get_unicodemap failed");

	if (ud.entry_ct != 2)
		kbd_error(EXIT_FAILURE, 0, "unexpected entry count: %u", ud.entry_ct);

	if (!ud.entries)
		kbd_error(EXIT_FAILURE, 0, "entries were not allocated");

	if (ud.entries[0].unicode != 0x41 || ud.entries[0].fontpos != 1)
		kbd_error(EXIT_FAILURE, 0, "unexpected first entry");

	if (ud.entries[1].unicode != 0x42 || ud.entries[1].fontpos != 2)
		kbd_error(EXIT_FAILURE, 0, "unexpected second entry");

	free(ud.entries);
	kfont_free(ctx);

	return EXIT_SUCCESS;
}
