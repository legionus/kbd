#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <linux/keyboard.h>

#include <keymap.h>
#include "contextP.h"
#include "libcommon.h"

static void
expect_contains(const char *buf, const char *needle)
{
	if (!strstr(buf, needle))
		kbd_error(EXIT_FAILURE, 0, "missing output fragment: %s", needle);
}

static void
test_dump_default_keymaps(void)
{
	struct lk_ctx *ctx;
	struct kbsentry kbs;
	struct lk_kbdiacr diacr;
	char *buf = NULL;
	size_t size = 0;
	FILE *fp;

	ctx = lk_init();
	if (!ctx)
		kbd_error(EXIT_FAILURE, 0, "Unable to initialize structure by valid pointer");

	lk_set_log_fn(ctx, NULL, NULL);

	if (lk_add_key(ctx, 0, 30, K(KT_LETTER, 'a')) != 0 ||
	    lk_add_key(ctx, 1, 30, K(KT_LETTER, 'A')) != 0 ||
	    lk_add_key(ctx, 8, 30, K(KT_META, 'a')) != 0 ||
	    lk_add_key(ctx, 0, 31, K(KT_LATIN, 'b')) != 0 ||
	    lk_add_key(ctx, 1, 31, K(KT_LATIN, 'x')) != 0 ||
	    lk_add_key(ctx, 8, 31, K(KT_META, 'b')) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to add key bindings");

	memset(&kbs, 0, sizeof(kbs));
	kbs.kb_func = 5;
	strcpy((char *)kbs.kb_string, "\"\\\n");
	if (lk_add_func(ctx, &kbs) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to add function string");

	diacr.diacr = '`';
	diacr.base = 'a';
	diacr.result = 0x00e0;
	if (lk_add_diacr(ctx, 0, &diacr) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to add first compose entry");

	diacr.diacr = '~';
	diacr.base = 'n';
	diacr.result = 0x1234;
	if (lk_add_diacr(ctx, 1, &diacr) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to add second compose entry");

	fp = open_memstream(&buf, &size);
	if (!fp)
		kbd_error(EXIT_FAILURE, 0, "Unable to allocate memory stream");

	lk_dump_keymap(ctx, fp, LK_SHAPE_DEFAULT, 0);
	lk_dump_diacs(ctx, fp);
	fclose(fp);

	expect_contains(buf, "keymaps 0-1,8\n");
	expect_contains(buf, "alt_is_meta\n");
	expect_contains(buf, "30 =");
	expect_contains(buf, "31 =");
	expect_contains(buf, "string F6 = \"\\\"\\\\\\012\"\n");
	expect_contains(buf, "compose '~' 'n' to 0x1234\n");

	free(buf);
	lk_free(ctx);
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

static void
test_dump_table_shapes(void)
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
}

static void
test_dump_c_and_binary_formats(void)
{
	struct lk_ctx *ctx;
	struct lk_ctx *badctx;
	struct kbsentry kbs;
	struct lk_kbdiacr diacr;
	char *buf = NULL;
	size_t size = 0;
	FILE *fp;
	unsigned char magic[7];

	ctx = lk_init();
	badctx = lk_init();
	if (!ctx || !badctx)
		kbd_error(EXIT_FAILURE, 0, "Unable to initialize structure by valid pointer");

	lk_set_log_fn(ctx, NULL, NULL);
	lk_set_log_fn(badctx, NULL, NULL);

	if (lk_add_key(ctx, 0, 1, K(KT_LATIN, 'c')) != 0 ||
	    lk_add_key(ctx, 1, 1, K(KT_LATIN, 'C')) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to add key bindings");

	memset(&kbs, 0, sizeof(kbs));
	kbs.kb_func = 0;
	strcpy((char *)kbs.kb_string, "abc");
	if (lk_add_func(ctx, &kbs) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to add function string");

	diacr.diacr = '^';
	diacr.base = 'o';
	diacr.result = 0x00f4;
	if (lk_add_diacr(ctx, 0, &diacr) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to add compose entry");

	fp = open_memstream(&buf, &size);
	if (!fp)
		kbd_error(EXIT_FAILURE, 0, "Unable to allocate memory stream");

	if (lk_dump_ctable(ctx, fp) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to dump C table");

	fclose(fp);

	expect_contains(buf, "unsigned short plain_map[NR_KEYS]");
	expect_contains(buf, "unsigned short shift_map[NR_KEYS]");
	expect_contains(buf, "char func_buf[] =");
	expect_contains(buf, "accent_table_size = 1;");
	free(buf);

	fp = tmpfile();
	if (!fp)
		kbd_error(EXIT_FAILURE, 0, "Unable to create temporary file");

	if (lk_dump_bkeymap(ctx, fp) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to dump binary keymap");

	rewind(fp);
	if (fread(magic, sizeof(magic), 1, fp) != 1)
		kbd_error(EXIT_FAILURE, 0, "Unable to read binary keymap header");

	if (memcmp(magic, "bkeymap", sizeof(magic)) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unexpected binary keymap header");

	fclose(fp);

	if (lk_add_key(badctx, 0, 3, 0x1ffff) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to add oversized key binding");

	fp = tmpfile();
	if (!fp)
		kbd_error(EXIT_FAILURE, 0, "Unable to create failure temporary file");

	if (lk_dump_bkeymap(badctx, fp) == 0)
		kbd_error(EXIT_FAILURE, 0, "Oversized key binding unexpectedly dumped");

	fclose(fp);
	lk_free(badctx);
	lk_free(ctx);
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	test_dump_default_keymaps();
	test_dump_table_shapes();
	test_dump_c_and_binary_formats();

	return EXIT_SUCCESS;
}
