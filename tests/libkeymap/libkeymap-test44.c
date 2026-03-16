#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <keymap.h>
#include "libcommon.h"

static void
expect_contains(const char *buf, const char *needle)
{
	if (!strstr(buf, needle))
		kbd_error(EXIT_FAILURE, 0, "missing output fragment: %s", needle);
}

static char *
dump_keys(struct lk_ctx *ctx, lk_table_shape table, char numeric)
{
	char *buf = NULL;
	size_t size = 0;
	FILE *fp = open_memstream(&buf, &size);

	if (!fp)
		kbd_error(EXIT_FAILURE, 0, "Unable to allocate memory stream");

	lk_dump_keymaps(ctx, fp);
	lk_dump_keys(ctx, fp, table, numeric);
	fclose(fp);

	return buf;
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct lk_ctx *ctx;
	char *buf;

	ctx = lk_init();
	if (!ctx)
		kbd_error(EXIT_FAILURE, 0, "Unable to initialize structure by valid pointer");

	lk_set_log_fn(ctx, NULL, NULL);

	if (lk_add_key(ctx, 0, 40, K(KT_FN, 5)) != 0 ||
	    lk_add_key(ctx, 1, 40, K(KT_META, 'a')) != 0 ||
	    lk_add_key(ctx, 2, 40, 0x1234) != 0 ||
	    lk_add_key(ctx, 0, 41, K(KT_LATIN, 'q')) != 0 ||
	    lk_add_key(ctx, 2, 41, K(KT_LATIN, 'z')) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to add key bindings");

	buf = dump_keys(ctx, LK_SHAPE_FULL_TABLE, 1);
	expect_contains(buf, "keymaps 0-2\n");
	expect_contains(buf, "keycode  40 =");
	expect_contains(buf, "0x");
	free(buf);

	buf = dump_keys(ctx, LK_SHAPE_SEPARATE_LINES, 0);
	expect_contains(buf, "plain\tkeycode  40 =");
	expect_contains(buf, "shift\tkeycode  40 =");
	expect_contains(buf, "altgr\tkeycode  40 =");
	free(buf);

	buf = dump_keys(ctx, LK_SHAPE_UNTIL_HOLE, 0);
	expect_contains(buf, "keycode  41 = q");
	expect_contains(buf, "altgr\tkeycode  41 = z");
	free(buf);

	lk_free(ctx);

	return EXIT_SUCCESS;
}
