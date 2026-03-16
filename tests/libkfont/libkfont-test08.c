#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <linux/kd.h>

#include <kfont.h>
#include "kfontP.h"
#include "libcommon.h"

static unsigned short *expected_map;
static struct unimapdesc *expected_ud;
static unsigned int clr_call_count;
static unsigned int put_call_count;
static unsigned int get_call_count;

static int
fake_ioctl_put_uniscrnmap(int fd, unsigned long req, void *arg)
{
	unsigned short *map = arg;
	size_t i;

	if (fd != 47 || req != PIO_UNISCRNMAP)
		kbd_error(EXIT_FAILURE, 0, "unexpected put_uniscrnmap call");

	if (map == expected_map)
		kbd_error(EXIT_FAILURE, 0, "ioctl received original map buffer");

	for (i = 0; i < E_TABSZ; i++) {
		if (map[i] != expected_map[i])
			kbd_error(EXIT_FAILURE, 0, "unexpected map entry at %zu", i);
	}

	return 0;
}

static int
fake_ioctl_put_unicodemap_retry(int fd, unsigned long req, void *arg)
{
	if (fd != 53)
		kbd_error(EXIT_FAILURE, 0, "unexpected fd: %d", fd);

	if (req == PIO_UNIMAPCLR) {
		struct unimapinit *ui = arg;
		if (ui->advised_hashlevel != clr_call_count + 2U)
			kbd_error(EXIT_FAILURE, 0, "unexpected advised_hashlevel");
		clr_call_count++;
		return 0;
	}

	if (req == PIO_UNIMAP) {
		struct unimapdesc *ud = arg;
		if (ud != expected_ud)
			kbd_error(EXIT_FAILURE, 0, "unexpected unimap descriptor");
		if (put_call_count < 2U) {
			put_call_count++;
			errno = ENOMEM;
			return -1;
		}
		put_call_count++;
		return 0;
	}

	return -1;
}

static int
fake_ioctl_put_unicodemap_fail(int fd, unsigned long req, void *arg)
{
	if (fd == 59 && req == PIO_UNIMAPCLR) {
		struct unimapinit *ui = arg;
		if (ui->advised_hashlevel != 99U + clr_call_count)
			kbd_error(EXIT_FAILURE, 0, "unexpected retry hashlevel");
		clr_call_count++;
		return 0;
	}

	if (fd == 59 && req == PIO_UNIMAP) {
		put_call_count++;
		errno = ENOMEM;
		return -1;
	}

	if (fd == 61 && req == PIO_UNIMAPCLR) {
		clr_call_count++;
		errno = EINVAL;
		return -1;
	}

	if (fd == 61 && req == PIO_UNIMAP)
		kbd_error(EXIT_FAILURE, 0, "PIO_UNIMAP should not be called after clear failure");

	return -1;
}

static int
fake_ioctl_get_uniscrnmap(int fd, unsigned long req, void *arg)
{
	unsigned short *map = arg;
	size_t i;

	if (fd != 67 || req != GIO_UNISCRNMAP)
		kbd_error(EXIT_FAILURE, 0, "unexpected get_uniscrnmap call");

	get_call_count++;
	for (i = 0; i < E_TABSZ; i++)
		map[i] = (unsigned short) ((0xf000U + i) & 0xffffU);
	return 0;
}

static void
test_put_uniscrnmap_copies_input(void)
{
	struct kfont_context *ctx;
	struct kfont_ops ops;
	unsigned short map[E_TABSZ];
	size_t i;

	if (kfont_init("libkfont-test08", &ctx) != 0)
		kbd_error(EXIT_FAILURE, 0, "unable to create kfont context");

	for (i = 0; i < E_TABSZ; i++)
		map[i] = (unsigned short) ((i * 17U) & 0xffffU);

	expected_map = map;
	kfont_set_logger(ctx, NULL);
	ops = ctx->ops;
	ops.ioctl_fn = fake_ioctl_put_uniscrnmap;
	kfont_set_ops(ctx, &ops);

	if (kfont_put_uniscrnmap(ctx, 47, map) != 0)
		kbd_error(EXIT_FAILURE, 0, "kfont_put_uniscrnmap failed");

	kfont_free(ctx);
}

static void
test_put_unicodemap_retry_then_success(void)
{
	struct kfont_context *ctx;
	struct kfont_ops ops;
	struct unimapinit ui = { .advised_hashsize = 11, .advised_hashstep = 7, .advised_hashlevel = 2 };
	struct unipair entries[2] = { { 0x41, 1 }, { 0x42, 2 } };
	struct unimapdesc ud = { .entry_ct = 2, .entries = entries };

	if (kfont_init("libkfont-test08", &ctx) != 0)
		kbd_error(EXIT_FAILURE, 0, "unable to create kfont context");

	clr_call_count = 0;
	put_call_count = 0;
	expected_ud = &ud;
	kfont_set_logger(ctx, NULL);
	ops = ctx->ops;
	ops.ioctl_fn = fake_ioctl_put_unicodemap_retry;
	kfont_set_ops(ctx, &ops);

	if (kfont_put_unicodemap(ctx, 53, &ui, &ud) != 0)
		kbd_error(EXIT_FAILURE, 0, "kfont_put_unicodemap failed");

	if (clr_call_count != 3U || put_call_count != 3U)
		kbd_error(EXIT_FAILURE, 0, "unexpected retry call counts");

	kfont_free(ctx);
}

static void
test_put_unicodemap_failure_paths(void)
{
	struct kfont_context *ctx;
	struct kfont_ops ops;
	struct unimapinit ui_retry = { .advised_hashsize = 3, .advised_hashstep = 5, .advised_hashlevel = 99 };
	struct unimapinit ui_clear = { .advised_hashsize = 13, .advised_hashstep = 9, .advised_hashlevel = 4 };
	struct unipair entries[1] = { { 0x41, 1 } };
	struct unimapdesc ud = { .entry_ct = 1, .entries = entries };

	if (kfont_init("libkfont-test08", &ctx) != 0)
		kbd_error(EXIT_FAILURE, 0, "unable to create kfont context");

	kfont_set_logger(ctx, NULL);
	ops = ctx->ops;

	clr_call_count = 0;
	put_call_count = 0;
	ops.ioctl_fn = fake_ioctl_put_unicodemap_fail;
	kfont_set_ops(ctx, &ops);
	if (kfont_put_unicodemap(ctx, 59, &ui_retry, &ud) == 0)
		kbd_error(EXIT_FAILURE, 0, "retry-limit failure unexpectedly succeeded");
	if (clr_call_count != 2U || put_call_count != 2U)
		kbd_error(EXIT_FAILURE, 0, "unexpected retry-limit call counts");

	clr_call_count = 0;
	put_call_count = 0;
	if (kfont_put_unicodemap(ctx, 61, &ui_clear, &ud) == 0)
		kbd_error(EXIT_FAILURE, 0, "clear failure unexpectedly succeeded");
	if (clr_call_count != 1U || put_call_count != 0U)
		kbd_error(EXIT_FAILURE, 0, "unexpected clear-failure call counts");

	kfont_free(ctx);
}

static void
test_get_uniscrnmap_wrapper(void)
{
	struct kfont_context *ctx;
	struct kfont_ops ops;
	unsigned short map[E_TABSZ];
	size_t i;

	if (kfont_init("libkfont-test08", &ctx) != 0)
		kbd_error(EXIT_FAILURE, 0, "unable to create kfont context");

	kfont_set_logger(ctx, NULL);
	ops = ctx->ops;
	ops.ioctl_fn = fake_ioctl_get_uniscrnmap;
	kfont_set_ops(ctx, &ops);
	memset(map, 0, sizeof(map));

	if (kfont_get_uniscrnmap(ctx, 67, map) != 0)
		kbd_error(EXIT_FAILURE, 0, "kfont_get_uniscrnmap failed");

	for (i = 0; i < E_TABSZ; i++) {
		if (map[i] != (unsigned short) ((0xf000U + i) & 0xffffU))
			kbd_error(EXIT_FAILURE, 0, "unexpected fetched map entry at %zu", i);
	}

	if (get_call_count != 1U)
		kbd_error(EXIT_FAILURE, 0, "unexpected get_uniscrnmap call count");

	kfont_free(ctx);
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	test_put_uniscrnmap_copies_input();
	test_put_unicodemap_retry_then_success();
	test_put_unicodemap_failure_paths();
	test_get_uniscrnmap_wrapper();
	return EXIT_SUCCESS;
}
