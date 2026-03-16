#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <linux/keyboard.h>

#include <keymap.h>
#include "libcommon.h"

static void
expect_contains(const char *buf, const char *needle)
{
	if (!strstr(buf, needle))
		kbd_error(EXIT_FAILURE, 0, "missing output fragment: %s", needle);
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
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

	return EXIT_SUCCESS;
}
