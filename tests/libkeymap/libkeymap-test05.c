#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <keymap.h>
#include "libcommon.h"

static void check_get_sym(struct lk_ctx *ctx, int ktyp, int index, const char *expect)
{
	char *s = lk_get_sym(ctx, ktyp, index);

	if (!s)
		kbd_error(EXIT_FAILURE, 0, "Unable to get key symbol with index %d", index);

	if (strcmp(s, expect))
		kbd_error(EXIT_FAILURE, 0, "Unexpected symbol at index %d", index);

	free(s);
}

static void check_code_to_ksym(struct lk_ctx *ctx, int code, const char *expect)
{
	char *s = lk_code_to_ksym(ctx, code);

	if (!s)
		kbd_error(EXIT_FAILURE, 0, "Unable to convert code (%d) to symbol", code);

	if (strcmp(s, expect))
		kbd_error(EXIT_FAILURE, 0, "Unexpected symbol '%s' for code %d", s, code);

	free(s);
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	int c;
	FILE *f = NULL;
	struct kbdfile *fp = NULL;
	struct kbdfile_ctx *kbdfile_ctx;
	struct lk_ctx *ctx;

	kbdfile_ctx = kbdfile_context_new();
	if (!kbdfile_ctx)
		kbd_error(EXIT_FAILURE, 0, "Unable to create kbdfile context");

	fp = kbdfile_new(kbdfile_ctx);
	if (!fp)
		kbd_error(EXIT_FAILURE, 0, "Unable to create kbdfile");

	ctx = lk_init();
	lk_set_log_fn(ctx, NULL, NULL);

	kbdfile_set_pathname(fp, "keymap0.map");

	f = fopen(TESTDIR "/data/libkeymap/keymap0.map", "r");
	if (!f)
		kbd_error(EXIT_FAILURE, 0, "Unable to open: " TESTDIR "/data/libkeymap/keymap0.map: %s", strerror(errno));

	kbdfile_set_file(fp, f);

	if (lk_parse_keymap(ctx, fp) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to parse keymap");

	c = lk_get_key(ctx, 0, 16);
	if (KVAL(c) != 'q')
		kbd_error(EXIT_FAILURE, 0, "Unable to get keycode 16");

	c = lk_get_key(ctx, 0, 17);
	if (KVAL(c) != 'w')
		kbd_error(EXIT_FAILURE, 0, "Unable to get keycode 17");

	c = lk_get_key(ctx, 0, 18);
	if (KVAL(c) != 'e')
		kbd_error(EXIT_FAILURE, 0, "Unable to get keycode 18");

	c = lk_get_key(ctx, 0, 19);
	if (KVAL(c) != 'r')
		kbd_error(EXIT_FAILURE, 0, "Unable to get keycode 19");

	c = lk_get_key(ctx, 0, 20);
	if (KVAL(c) != 't')
		kbd_error(EXIT_FAILURE, 0, "Unable to get keycode 20");

	c = lk_get_key(ctx, 0, 21);
	if (KVAL(c) != 'y')
		kbd_error(EXIT_FAILURE, 0, "Unable to get keycode 21");

	if (lk_get_sym(ctx, 1024, 1) != NULL)
		kbd_error(EXIT_FAILURE, 0, "Unexpected return for unknown ktype");

	if (lk_get_sym(ctx, KT_LATIN, 512) != NULL)
		kbd_error(EXIT_FAILURE, 0, "Unexpected return for index out of range");

	check_get_sym(ctx, KT_LATIN, 65, "A");
	check_get_sym(ctx, KT_LATIN, 66, "B");
	check_get_sym(ctx, KT_LATIN, 67, "C");

	check_code_to_ksym(ctx, K(KT_LATIN, 65), "A");
	check_code_to_ksym(ctx, K(KT_LATIN, 66), "B");
	check_code_to_ksym(ctx, K(KT_LATIN, 67), "C");

	if (lk_convert_code(ctx, (int) 'A', TO_AUTO) != 'A')
		kbd_error(EXIT_FAILURE, 0, "Unexpected code after conversion of 'A'");

	kbdfile_free(fp);
	kbdfile_context_free(kbdfile_ctx);
	lk_free(ctx);

	return EXIT_SUCCESS;
}
