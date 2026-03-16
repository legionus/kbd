#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <sys/stat.h>

#include <linux/kd.h>

#include <kfont.h>
#include "kfontP.h"
#include "libcommon.h"

#define TEST_CONSOLE_FD 71

enum test_mode {
	TEST_MODE_DIRECT_MAP = 0,
	TEST_MODE_UNICODE_ONLY = 1,
};

static enum test_mode current_mode;
static unsigned int gio_scrnmap_calls;
static unsigned int gio_uniscrnmap_calls;

static int
fake_ioctl(int fd, unsigned long req, void *arg)
{
	unsigned int i;

	if (fd != TEST_CONSOLE_FD)
		kbd_error(EXIT_FAILURE, 0, "unexpected fd: %d", fd);

	if (req == GIO_SCRNMAP) {
		unsigned char *map = arg;

		gio_scrnmap_calls++;

		if (current_mode == TEST_MODE_UNICODE_ONLY) {
			errno = EINVAL;
			return -1;
		}

		/*
		 * This is the legacy compact console map. If the unicode map also
		 * contains only direct-font values, save_consolemap() should prefer
		 * writing these 256 bytes instead of the 512-byte unicode form.
		 */
		for (i = 0; i < E_TABSZ; i++)
			map[i] = (unsigned char)((255U - i) & 0xffU);

		return 0;
	}

	if (req == GIO_UNISCRNMAP) {
		unsigned short *map = arg;

		gio_uniscrnmap_calls++;

		if (current_mode == TEST_MODE_DIRECT_MAP) {
			for (i = 0; i < E_TABSZ; i++)
				map[i] = (unsigned short)(0xf000U + i);
		} else {
			/*
			 * Non-direct values force kfont_save_consolemap() to write the
			 * full unicode screen map, because the 8-bit map cannot represent
			 * these codepoints.
			 */
			for (i = 0; i < E_TABSZ; i++)
				map[i] = (unsigned short)(0x0400U + i);
		}

		return 0;
	}

	kbd_error(EXIT_FAILURE, 0, "unexpected request: %lu", req);
	return -1;
}

static void
read_file(const char *path, void *buf, size_t size)
{
	FILE *fp;

	fp = fopen(path, "rb");
	if (!fp)
		kbd_error(EXIT_FAILURE, errno, "fopen failed for %s", path);

	if (fread(buf, size, 1, fp) != 1)
		kbd_error(EXIT_FAILURE, errno, "fread failed for %s", path);

	fclose(fp);
}

static void
check_file_size(const char *path, off_t expected_size)
{
	struct stat st;

	if (stat(path, &st) != 0)
		kbd_error(EXIT_FAILURE, errno, "stat failed for %s", path);

	if (st.st_size != expected_size)
		kbd_error(EXIT_FAILURE, 0, "unexpected file size for %s: %lld",
				path, (long long)st.st_size);
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct kfont_context *ctx;
	struct kfont_ops ops;
	char direct_path[] = "/tmp/libkfont-test19-direct-XXXXXX";
	char unicode_path[] = "/tmp/libkfont-test19-unicode-XXXXXX";
	unsigned char direct_map[E_TABSZ];
	unsigned short unicode_map[E_TABSZ];
	int fd;
	unsigned int i;

	if (kfont_init("libkfont-test19", &ctx) != 0)
		kbd_error(EXIT_FAILURE, 0, "unable to create kfont context");

	kfont_set_logger(ctx, NULL);

	ops = ctx->ops;
	ops.ioctl_fn = fake_ioctl;
	kfont_set_ops(ctx, &ops);

	fd = mkstemp(direct_path);
	if (fd < 0)
		kbd_error(EXIT_FAILURE, errno, "mkstemp failed");
	close(fd);

	fd = mkstemp(unicode_path);
	if (fd < 0)
		kbd_error(EXIT_FAILURE, errno, "mkstemp failed");
	close(fd);

	current_mode = TEST_MODE_DIRECT_MAP;
	if (kfont_save_consolemap(ctx, TEST_CONSOLE_FD, direct_path) != 0)
		kbd_error(EXIT_FAILURE, 0, "kfont_save_consolemap failed for direct map");

	check_file_size(direct_path, E_TABSZ);
	read_file(direct_path, direct_map, sizeof(direct_map));

	for (i = 0; i < E_TABSZ; i++) {
		unsigned char expected = (unsigned char)((255U - i) & 0xffU);

		if (direct_map[i] != expected)
			kbd_error(EXIT_FAILURE, 0, "unexpected direct map byte at %u: %#x",
					i, direct_map[i]);
	}

	current_mode = TEST_MODE_UNICODE_ONLY;
	if (kfont_save_consolemap(ctx, TEST_CONSOLE_FD, unicode_path) != 0)
		kbd_error(EXIT_FAILURE, 0, "kfont_save_consolemap failed for unicode map");

	check_file_size(unicode_path, 2 * E_TABSZ);
	read_file(unicode_path, unicode_map, sizeof(unicode_map));

	for (i = 0; i < E_TABSZ; i++) {
		unsigned short expected = (unsigned short)(0x0400U + i);

		if (unicode_map[i] != expected)
			kbd_error(EXIT_FAILURE, 0, "unexpected unicode map value at %u: %#x",
					i, unicode_map[i]);
	}

	if (gio_scrnmap_calls != 2U)
		kbd_error(EXIT_FAILURE, 0, "unexpected GIO_SCRNMAP call count: %u",
				gio_scrnmap_calls);

	if (gio_uniscrnmap_calls != 2U)
		kbd_error(EXIT_FAILURE, 0, "unexpected GIO_UNISCRNMAP call count: %u",
				gio_uniscrnmap_calls);

	unlink(direct_path);
	unlink(unicode_path);
	kfont_free(ctx);

	return EXIT_SUCCESS;
}
