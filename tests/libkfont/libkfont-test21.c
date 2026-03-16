#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>

#include <linux/kd.h>

#include "kfontP.h"
#include "libcommon.h"

#define TEST_CONSOLE_FD 79

static unsigned int pio_scrnmap_calls;

static int
fake_ioctl(int fd, unsigned long req, void *arg)
{
	unsigned char *map = arg;
	unsigned int i;

	if (fd != TEST_CONSOLE_FD)
		kbd_error(EXIT_FAILURE, 0, "unexpected fd: %d", fd);

	if (req != PIO_SCRNMAP)
		kbd_error(EXIT_FAILURE, 0, "unexpected request: %lu", req);

	pio_scrnmap_calls++;

	for (i = 0; i < E_TABSZ; i++) {
		unsigned char expected = (unsigned char)((i * 7U) & 0xffU);

		if (map[i] != expected)
			kbd_error(EXIT_FAILURE, 0, "unexpected map[%u]: %#x", i, map[i]);
	}

	return 0;
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct kfont_context *ctx;
	struct kfont_ops ops;
	char path[] = "/tmp/libkfont-test21-XXXXXX";
	unsigned char map[E_TABSZ];
	FILE *fp;
	int fd;
	unsigned int i;

	for (i = 0; i < E_TABSZ; i++)
		map[i] = (unsigned char)((i * 7U) & 0xffU);

	fd = mkstemp(path);
	if (fd < 0)
		kbd_error(EXIT_FAILURE, errno, "mkstemp failed");

	fp = fdopen(fd, "wb");
	if (!fp)
		kbd_error(EXIT_FAILURE, errno, "fdopen failed");

	/* A file of exactly E_TABSZ bytes is treated as a binary direct screen map. */
	if (fwrite(map, sizeof(map), 1, fp) != 1)
		kbd_error(EXIT_FAILURE, errno, "unable to write fixture");

	fclose(fp);

	if (kfont_init("libkfont-test21", &ctx) != 0)
		kbd_error(EXIT_FAILURE, 0, "unable to create kfont context");

	kfont_set_logger(ctx, NULL);

	ops = ctx->ops;
	ops.ioctl_fn = fake_ioctl;
	kfont_set_ops(ctx, &ops);

	if (kfont_load_consolemap(ctx, TEST_CONSOLE_FD, path) != 0)
		kbd_error(EXIT_FAILURE, 0, "kfont_load_consolemap failed");

	if (pio_scrnmap_calls != 1U)
		kbd_error(EXIT_FAILURE, 0, "unexpected PIO_SCRNMAP call count: %u",
				pio_scrnmap_calls);

	unlink(path);
	kfont_free(ctx);

	return EXIT_SUCCESS;
}
