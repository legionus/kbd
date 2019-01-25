#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <error.h>
#include <errno.h>

#include <keymap.h>

int
main(void)
{
	unsigned int i = 0;
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

	kbdfile_set_pathname(fp, "keymap2.map");

	f = fopen(DATADIR "/data/libkeymap/keymap2.map", "r");
	if(!f)
		error(EXIT_FAILURE, 0, "Unable to open: " DATADIR "/data/libkeymap/keymap2.map: %s", strerror(errno));

	kbdfile_set_file(fp, f);

	if (lk_parse_keymap(ctx, fp) != 0)
		error(EXIT_FAILURE, 0, "Unable to parse keymap");

	while (i < MAX_NR_KEYMAPS) {
		int c = lk_get_key(ctx, i, 17);
		if (KVAL(c) != 'x')
			error(EXIT_FAILURE, 0, "Unable to get keycode");
		i++;
	}

	kbdfile_free(fp);
	kbdfile_context_free(kbdfile_ctx);
	lk_free(ctx);

	return EXIT_SUCCESS;
}
