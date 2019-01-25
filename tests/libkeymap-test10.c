#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <error.h>
#include <errno.h>

#include <keymap.h>

int
main(void)
{
	unsigned int i;
	FILE *f = NULL;
	struct kbdfile *fp = NULL;
	struct kbdfile_ctx *kbdfile_ctx;
	struct kbsentry kbs;
	struct lk_ctx *ctx;

	kbdfile_ctx = kbdfile_context_new();
	if (!kbdfile_ctx)
		error(EXIT_FAILURE, 0, "Unable to create kbdfile context");

	fp = kbdfile_new(kbdfile_ctx);
	if (!fp)
		error(EXIT_FAILURE, 0, "Unable to create kbdfile");

	ctx = lk_init();
	lk_set_log_fn(ctx, NULL, NULL);

	kbdfile_set_pathname(fp, "keymap5.map");

	f = fopen(DATADIR "/data/libkeymap/keymap5.map", "r");
	if (!f)
		error(EXIT_FAILURE, 0, "Unable to open: " DATADIR "/data/libkeymap/keymap5.map: %s", strerror(errno));

	kbdfile_set_file(fp, f);

	if (lk_parse_keymap(ctx, fp) != 0)
		error(EXIT_FAILURE, 0, "Unable to parse keymap");

	for (i = 0; i < MAX_NR_FUNC; i++) {
		kbs.kb_func      = (unsigned char) i;
		kbs.kb_string[0] = 0;
		if (lk_get_func(ctx, &kbs) != 0)
			error(EXIT_FAILURE, 0, "Unable to get func %d", i);
	}

	kbdfile_free(fp);
	kbdfile_context_free(kbdfile_ctx);
	lk_free(ctx);

	return EXIT_SUCCESS;
}
