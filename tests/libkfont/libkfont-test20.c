#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <errno.h>
#include <sys/stat.h>

#include <linux/kd.h>

#include <kfont.h>
#include "kfontP.h"
#include "libcommon.h"

#define TEST_CONSOLE_FD 73

static unsigned int gio_scrnmap_calls;
static unsigned int gio_uniscrnmap_calls;

static int
fake_ioctl(int fd, unsigned long req, void *arg KBD_ATTR_UNUSED)
{
	if (fd != TEST_CONSOLE_FD)
		kbd_error(EXIT_FAILURE, 0, "unexpected fd: %d", fd);

	if (req == GIO_SCRNMAP) {
		gio_scrnmap_calls++;
		errno = EINVAL;
		return -1;
	}

	if (req == GIO_UNISCRNMAP) {
		gio_uniscrnmap_calls++;
		errno = EINVAL;
		return -1;
	}

	kbd_error(EXIT_FAILURE, 0, "unexpected request: %lu", req);
	return -1;
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct kfont_context *ctx;
	struct kfont_ops ops;
	char path[] = "/tmp/libkfont-test20-XXXXXX";
	struct stat st;
	int fd;
	int ret;

	if (kfont_init("libkfont-test20", &ctx) != 0)
		kbd_error(EXIT_FAILURE, 0, "unable to create kfont context");

	kfont_set_logger(ctx, NULL);

	ops = ctx->ops;
	ops.ioctl_fn = fake_ioctl;
	kfont_set_ops(ctx, &ops);

	fd = mkstemp(path);
	if (fd < 0)
		kbd_error(EXIT_FAILURE, errno, "mkstemp failed");
	close(fd);

	/*
	 * If neither the legacy screen map nor the unicode screen map can be read,
	 * kfont_save_consolemap() should report failure instead of writing garbage.
	 */
	ret = kfont_save_consolemap(ctx, TEST_CONSOLE_FD, path);
	if (ret == 0)
		kbd_error(EXIT_FAILURE, 0, "expected kfont_save_consolemap to fail");

	if (gio_scrnmap_calls != 1U)
		kbd_error(EXIT_FAILURE, 0, "unexpected GIO_SCRNMAP call count: %u",
				gio_scrnmap_calls);

	if (gio_uniscrnmap_calls != 1U)
		kbd_error(EXIT_FAILURE, 0, "unexpected GIO_UNISCRNMAP call count: %u",
				gio_uniscrnmap_calls);

	if (stat(path, &st) != 0)
		kbd_error(EXIT_FAILURE, errno, "stat failed");

	if (st.st_size != 0)
		kbd_error(EXIT_FAILURE, 0, "unexpected output size on failure: %lld",
				(long long)st.st_size);

	unlink(path);
	kfont_free(ctx);

	return EXIT_SUCCESS;
}
