#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <linux/kd.h>

#include "kfontP.h"
#include "libcommon.h"

#define TEST_CONSOLE_FD 59

static int
fake_ioctl(int fd, unsigned long req, void *arg)
{
	unsigned short *map = arg;
	unsigned int i;

	if (fd != TEST_CONSOLE_FD)
		kbd_error(EXIT_FAILURE, 0, "unexpected fd: %d", fd);

	if (req != PIO_UNISCRNMAP)
		kbd_error(EXIT_FAILURE, 0, "unexpected request: %lu", req);

	if (map[0] != 0x0041)
		kbd_error(EXIT_FAILURE, 0, "unexpected map[0]: %#x", map[0]);

	if (map[1] != 0x0042)
		kbd_error(EXIT_FAILURE, 0, "unexpected map[1]: %#x", map[1]);

	if (map[2] != 0x0043)
		kbd_error(EXIT_FAILURE, 0, "unexpected map[2]: %#x", map[2]);

	if (map[3] != 0x2603)
		kbd_error(EXIT_FAILURE, 0, "unexpected map[3]: %#x", map[3]);

	for (i = 4; i < E_TABSZ; i++) {
		unsigned short expected = (unsigned short)(0xf000U + i);

		if (map[i] != expected)
			kbd_error(EXIT_FAILURE, 0, "unexpected default map[%u]: %#x", i, map[i]);
	}

	return 0;
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct kfont_context *ctx;
	struct kfont_ops ops;
	char path[] = "/tmp/libkfont-test14-XXXXXX";
	FILE *fp;
	int fd;

	fd = mkstemp(path);
	if (fd < 0)
		kbd_error(EXIT_FAILURE, 0, "mkstemp failed");

	fp = fdopen(fd, "w");
	if (!fp)
		kbd_error(EXIT_FAILURE, 0, "fdopen failed");

	if (fputs("0 U+0041\n1 0x42\n2 'C'\n3 U+2603\n", fp) == EOF)
		kbd_error(EXIT_FAILURE, 0, "unable to write console map fixture");

	fclose(fp);

	if (kfont_init("libkfont-test14", &ctx) != 0)
		kbd_error(EXIT_FAILURE, 0, "unable to create kfont context");

	kfont_set_logger(ctx, NULL);

	ops = ctx->ops;
	ops.ioctl_fn = fake_ioctl;
	kfont_set_ops(ctx, &ops);

	if (kfont_load_consolemap(ctx, TEST_CONSOLE_FD, path) != 0)
		kbd_error(EXIT_FAILURE, 0, "kfont_load_consolemap failed");

	unlink(path);
	kfont_free(ctx);

	return EXIT_SUCCESS;
}
