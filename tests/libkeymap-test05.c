#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <error.h>
#include <errno.h>

#include <keymap.h>

int
main(void)
{
	int c;
	FILE *f = NULL;
	struct kbdfile *fp = NULL;
	struct kbdfile_ctx *kbdfile_ctx;
	struct lk_ctx *ctx;

	kbdfile_ctx = kbdfile_context_new();
	if (!kbdfile_ctx)
		error(EXIT_FAILURE, 0, "Unable to create kbdfile context");

	fp = kbdfile_new(kbdfile_ctx);
	if (!fp)
		error(EXIT_FAILURE, 0, "Unable to create kbdfile");

	ctx = lk_init();
	lk_set_log_fn(ctx, NULL, NULL);

	kbdfile_set_pathname(fp, "keymap0.map");

	f = fopen(DATADIR "/data/libkeymap/keymap0.map", "r");
	if (!f)
		error(EXIT_FAILURE, 0, "Unable to open: " DATADIR "/data/libkeymap/keymap0.map: %s", strerror(errno));

	kbdfile_set_file(fp, f);

	if (lk_parse_keymap(ctx, fp) != 0)
		error(EXIT_FAILURE, 0, "Unable to parse keymap");

	c = lk_get_key(ctx, 0, 16);
	if (KVAL(c) != 'q')
		error(EXIT_FAILURE, 0, "Unable to get keycode 16");

	c = lk_get_key(ctx, 0, 17);
	if (KVAL(c) != 'w')
		error(EXIT_FAILURE, 0, "Unable to get keycode 17");

	c = lk_get_key(ctx, 0, 18);
	if (KVAL(c) != 'e')
		error(EXIT_FAILURE, 0, "Unable to get keycode 18");

	c = lk_get_key(ctx, 0, 19);
	if (KVAL(c) != 'r')
		error(EXIT_FAILURE, 0, "Unable to get keycode 19");

	c = lk_get_key(ctx, 0, 20);
	if (KVAL(c) != 't')
		error(EXIT_FAILURE, 0, "Unable to get keycode 20");

	c = lk_get_key(ctx, 0, 21);
	if (KVAL(c) != 'y')
		error(EXIT_FAILURE, 0, "Unable to get keycode 21");

	kbdfile_free(fp);
	kbdfile_context_free(kbdfile_ctx);
	lk_free(ctx);

	return EXIT_SUCCESS;
}
