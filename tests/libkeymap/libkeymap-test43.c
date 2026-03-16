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

	return EXIT_SUCCESS;
}
