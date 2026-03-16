#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <sysexits.h>
#include <sys/stat.h>

#include <linux/kd.h>

#include <kfont.h>
#include "kfontP.h"
#include "libcommon.h"

static unsigned int gio_scrnmap_calls;
static unsigned int gio_uniscrnmap_calls;
static unsigned int current_mode;

static int
fake_ioctl_save_consolemap(int fd, unsigned long req, void *arg)
{
	unsigned int i;

	if (fd == 71 && req == GIO_SCRNMAP) {
		unsigned char *map = arg;
		gio_scrnmap_calls++;
		if (current_mode == 1) {
			errno = EINVAL;
			return -1;
		}
		for (i = 0; i < E_TABSZ; i++)
			map[i] = (unsigned char) ((255U - i) & 0xffU);
		return 0;
	}

	if (fd == 71 && req == GIO_UNISCRNMAP) {
		unsigned short *map = arg;
		gio_uniscrnmap_calls++;
		for (i = 0; i < E_TABSZ; i++)
			map[i] = (unsigned short) ((current_mode == 0 ? 0xf000U : 0x0400U) + i);
		return 0;
	}

	if (fd == 73 && req == GIO_SCRNMAP) {
		gio_scrnmap_calls++;
		errno = EINVAL;
		return -1;
	}

	if (fd == 73 && req == GIO_UNISCRNMAP) {
		gio_uniscrnmap_calls++;
		errno = EINVAL;
		return -1;
	}

	if (fd == 89 && req == GIO_SCRNMAP) {
		gio_scrnmap_calls++;
		for (i = 0; i < E_TABSZ; i++)
			((unsigned char *) arg)[i] = (unsigned char) i;
		return 0;
	}

	if (fd == 89 && req == GIO_UNISCRNMAP) {
		gio_uniscrnmap_calls++;
		for (i = 0; i < E_TABSZ; i++)
			((unsigned short *) arg)[i] = (unsigned short) (0xf000U + i);
		return 0;
	}

	return -1;
}

static void
read_file(const char *path, void *buf, size_t size)
{
	FILE *fp = fopen(path, "rb");

	if (!fp)
		kbd_error(EXIT_FAILURE, errno, "fopen failed for %s", path);

	if (fread(buf, size, 1, fp) != 1)
		kbd_error(EXIT_FAILURE, errno, "fread failed for %s", path);

	fclose(fp);
}

static void
test_save_consolemap_variants(void)
{
	struct kfont_context *ctx;
	struct kfont_ops ops;
	char direct_path[] = "/tmp/libkfont-test19-direct-XXXXXX";
	char unicode_path[] = "/tmp/libkfont-test19-unicode-XXXXXX";
	unsigned char direct_map[E_TABSZ];
	unsigned short unicode_map[E_TABSZ];
	struct stat st;
	int fd;
	unsigned int i;

	if (kfont_init("libkfont-test19", &ctx) != 0)
		kbd_error(EXIT_FAILURE, 0, "unable to create kfont context");

	kfont_set_logger(ctx, NULL);
	ops = ctx->ops;
	ops.ioctl_fn = fake_ioctl_save_consolemap;
	kfont_set_ops(ctx, &ops);

	fd = mkstemp(direct_path);
	if (fd < 0)
		kbd_error(EXIT_FAILURE, errno, "mkstemp failed");
	close(fd);

	fd = mkstemp(unicode_path);
	if (fd < 0)
		kbd_error(EXIT_FAILURE, errno, "mkstemp failed");
	close(fd);

	gio_scrnmap_calls = 0;
	gio_uniscrnmap_calls = 0;
	current_mode = 0;
	if (kfont_save_consolemap(ctx, 71, direct_path) != 0)
		kbd_error(EXIT_FAILURE, 0, "kfont_save_consolemap failed for direct map");
	if (stat(direct_path, &st) != 0 || st.st_size != E_TABSZ)
		kbd_error(EXIT_FAILURE, 0, "unexpected direct map file size");
	read_file(direct_path, direct_map, sizeof(direct_map));
	for (i = 0; i < E_TABSZ; i++) {
		if (direct_map[i] != (unsigned char) ((255U - i) & 0xffU))
			kbd_error(EXIT_FAILURE, 0, "unexpected direct map byte at %u", i);
	}

	current_mode = 1;
	if (kfont_save_consolemap(ctx, 71, unicode_path) != 0)
		kbd_error(EXIT_FAILURE, 0, "kfont_save_consolemap failed for unicode map");
	if (stat(unicode_path, &st) != 0 || st.st_size != 2 * E_TABSZ)
		kbd_error(EXIT_FAILURE, 0, "unexpected unicode map file size");
	read_file(unicode_path, unicode_map, sizeof(unicode_map));
	for (i = 0; i < E_TABSZ; i++) {
		if (unicode_map[i] != (unsigned short) (0x0400U + i))
			kbd_error(EXIT_FAILURE, 0, "unexpected unicode map value at %u", i);
	}

	unlink(direct_path);
	unlink(unicode_path);
	kfont_free(ctx);
}

static void
test_save_consolemap_failures(void)
{
	struct kfont_context *ctx;
	struct kfont_ops ops;
	char path[] = "/tmp/libkfont-test19-fail-XXXXXX";
	struct stat st;
	int fd;
	int ret;

	if (kfont_init("libkfont-test19", &ctx) != 0)
		kbd_error(EXIT_FAILURE, 0, "unable to create kfont context");

	kfont_set_logger(ctx, NULL);
	ops = ctx->ops;
	ops.ioctl_fn = fake_ioctl_save_consolemap;
	kfont_set_ops(ctx, &ops);

	fd = mkstemp(path);
	if (fd < 0)
		kbd_error(EXIT_FAILURE, errno, "mkstemp failed");
	close(fd);

	gio_scrnmap_calls = 0;
	gio_uniscrnmap_calls = 0;
	ret = kfont_save_consolemap(ctx, 73, path);
	if (ret == 0)
		kbd_error(EXIT_FAILURE, 0, "expected save_consolemap to fail");
	if (stat(path, &st) != 0 || st.st_size != 0)
		kbd_error(EXIT_FAILURE, 0, "unexpected output size after read failure");

	ret = kfont_save_consolemap(ctx, 89, "/tmp/no-such-dir/consolemap.out");
	if (ret != -EX_DATAERR)
		kbd_error(EXIT_FAILURE, 0, "unexpected fopen failure result: %d", ret);

	unlink(path);
	kfont_free(ctx);
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	test_save_consolemap_variants();
	test_save_consolemap_failures();
	return EXIT_SUCCESS;
}
