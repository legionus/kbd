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

	setenv("LOADKEYS_INCLUDE_PATH", DATADIR "/data/libkeymap", 1);

	kbdfile_ctx = kbdfile_context_new();
	if (!kbdfile_ctx)
		error(EXIT_FAILURE, 0, "Unable to create kbdfile context");

	fp = kbdfile_new(kbdfile_ctx);
	if (!fp)
		error(EXIT_FAILURE, 0, "Unable to create kbdfile");

	ctx = lk_init();
	lk_set_log_fn(ctx, NULL, NULL);

	kbdfile_set_pathname(fp, "keymap4.map");

	f = fopen(DATADIR "/data/libkeymap/keymap4.map", "r");
	if (!f)
		error(EXIT_FAILURE, 0, "Unable to open: " DATADIR "/data/libkeymap/keymap4.map: %s", strerror(errno));

	kbdfile_set_file(fp, f);

	if (lk_parse_keymap(ctx, fp) != 0)
		error(EXIT_FAILURE, 0, "Unable to parse keymap");

	c = lk_get_key(ctx, 0, 16);
	if (KVAL(c) != 'q')
		error(EXIT_FAILURE, 0, "Unable to get keycode");

	c = lk_get_key(ctx, 0, 17);
	if (KVAL(c) != 'w')
		error(EXIT_FAILURE, 0, "Include40.map failed");

	c = lk_get_key(ctx, 0, 18);
	if (KVAL(c) != 'e')
		error(EXIT_FAILURE, 0, "Include41.map failed");

	kbdfile_free(fp);
	kbdfile_context_free(kbdfile_ctx);
	lk_free(ctx);

	return EXIT_SUCCESS;
}
