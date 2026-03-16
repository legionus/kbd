#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>

#include <linux/kd.h>

#include "kfontP.h"
#include "libcommon.h"

static unsigned int pio_scrnmap_calls;
static unsigned int pio_uniscrnmap_calls;

static int
fake_ioctl_load_symbolic_map(int fd, unsigned long req, void *arg)
{
	unsigned short *map = arg;
	unsigned int i;

	if (fd != 59 || req != PIO_UNISCRNMAP)
		kbd_error(EXIT_FAILURE, 0, "unexpected symbolic console map ioctl");

	if (map[0] != 0x0041 || map[1] != 0x0042 || map[2] != 0x0043 || map[3] != 0x2603)
		kbd_error(EXIT_FAILURE, 0, "unexpected parsed symbolic map values");

	for (i = 4; i < E_TABSZ; i++) {
		if (map[i] != (unsigned short) (0xf000U + i))
			kbd_error(EXIT_FAILURE, 0, "unexpected symbolic default map[%u]", i);
	}

	return 0;
}

static int
fake_ioctl_load_binary_maps(int fd, unsigned long req, void *arg)
{
	unsigned int i;

	if (fd == 79 && req == PIO_SCRNMAP) {
		unsigned char *map = arg;
		pio_scrnmap_calls++;
		for (i = 0; i < E_TABSZ; i++) {
			if (map[i] != (unsigned char) ((i * 7U) & 0xffU))
				kbd_error(EXIT_FAILURE, 0, "unexpected direct map[%u]", i);
		}
		return 0;
	}

	if (fd == 83 && req == PIO_UNISCRNMAP) {
		unsigned short *map = arg;
		pio_uniscrnmap_calls++;
		for (i = 0; i < E_TABSZ; i++) {
			if (map[i] != (unsigned short) (0x2500U + i))
				kbd_error(EXIT_FAILURE, 0, "unexpected unicode map[%u]", i);
		}
		return 0;
	}

	return -1;
}

static void
test_load_symbolic_consolemap(void)
{
	struct kfont_context *ctx;
	struct kfont_ops ops;
	char path[] = "/tmp/libkfont-test14-symbolic-XXXXXX";
	FILE *fp;
	int fd;

	fd = mkstemp(path);
	if (fd < 0)
		kbd_error(EXIT_FAILURE, errno, "mkstemp failed");

	fp = fdopen(fd, "w");
	if (!fp)
		kbd_error(EXIT_FAILURE, errno, "fdopen failed");

	if (fputs("0 U+0041\n1 0x42\n2 'C'\n3 U+2603\n", fp) == EOF)
		kbd_error(EXIT_FAILURE, errno, "unable to write console map fixture");
	fclose(fp);

	if (kfont_init("libkfont-test14", &ctx) != 0)
		kbd_error(EXIT_FAILURE, 0, "unable to create kfont context");

	kfont_set_logger(ctx, NULL);
	ops = ctx->ops;
	ops.ioctl_fn = fake_ioctl_load_symbolic_map;
	kfont_set_ops(ctx, &ops);

	if (kfont_load_consolemap(ctx, 59, path) != 0)
		kbd_error(EXIT_FAILURE, 0, "kfont_load_consolemap failed");

	unlink(path);
	kfont_free(ctx);
}

static void
test_load_binary_consolemaps(void)
{
	struct kfont_context *ctx;
	struct kfont_ops ops;
	char direct_path[] = "/tmp/libkfont-test14-direct-XXXXXX";
	char unicode_path[] = "/tmp/libkfont-test14-unicode-XXXXXX";
	unsigned char direct_map[E_TABSZ];
	unsigned short unicode_map[E_TABSZ];
	FILE *fp;
	int fd;
	unsigned int i;

	for (i = 0; i < E_TABSZ; i++) {
		direct_map[i] = (unsigned char) ((i * 7U) & 0xffU);
		unicode_map[i] = (unsigned short) (0x2500U + i);
	}

	fd = mkstemp(direct_path);
	if (fd < 0)
		kbd_error(EXIT_FAILURE, errno, "mkstemp failed");
	fp = fdopen(fd, "wb");
	if (!fp)
		kbd_error(EXIT_FAILURE, errno, "fdopen failed");
	if (fwrite(direct_map, sizeof(direct_map), 1, fp) != 1)
		kbd_error(EXIT_FAILURE, errno, "unable to write direct map fixture");
	fclose(fp);

	fd = mkstemp(unicode_path);
	if (fd < 0)
		kbd_error(EXIT_FAILURE, errno, "mkstemp failed");
	fp = fdopen(fd, "wb");
	if (!fp)
		kbd_error(EXIT_FAILURE, errno, "fdopen failed");
	if (fwrite(unicode_map, sizeof(unicode_map), 1, fp) != 1)
		kbd_error(EXIT_FAILURE, errno, "unable to write unicode map fixture");
	fclose(fp);

	if (kfont_init("libkfont-test14", &ctx) != 0)
		kbd_error(EXIT_FAILURE, 0, "unable to create kfont context");

	pio_scrnmap_calls = 0;
	pio_uniscrnmap_calls = 0;
	kfont_set_logger(ctx, NULL);
	ops = ctx->ops;
	ops.ioctl_fn = fake_ioctl_load_binary_maps;
	kfont_set_ops(ctx, &ops);

	if (kfont_load_consolemap(ctx, 79, direct_path) != 0)
		kbd_error(EXIT_FAILURE, 0, "kfont_load_consolemap failed for direct map");
	if (kfont_load_consolemap(ctx, 83, unicode_path) != 0)
		kbd_error(EXIT_FAILURE, 0, "kfont_load_consolemap failed for unicode map");

	if (pio_scrnmap_calls != 1U || pio_uniscrnmap_calls != 1U)
		kbd_error(EXIT_FAILURE, 0, "unexpected load call counts");

	unlink(direct_path);
	unlink(unicode_path);
	kfont_free(ctx);
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	test_load_symbolic_consolemap();
	test_load_binary_consolemaps();
	return EXIT_SUCCESS;
}
