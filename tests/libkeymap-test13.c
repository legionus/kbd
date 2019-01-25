#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <error.h>
#include <errno.h>

#include <keymap.h>

int
main(void)
{
	struct lk_ctx *ctx;
	struct kmapinfo info;

	ctx = lk_init();
	lk_set_log_fn(ctx, NULL, NULL);

	if (lk_add_map(ctx, 0) != 0)
		error(EXIT_FAILURE, 0, "Unable to define map");

	lk_get_kmapinfo(ctx, &info);

	if (info.keymaps != 1)
		error(EXIT_FAILURE, 0, "Wrong keymap number");

	if (lk_add_map(ctx, 0) != 0)
		error(EXIT_FAILURE, 0, "Unable to define map");

	lk_get_kmapinfo(ctx, &info);

	if (info.keymaps != 1)
		error(EXIT_FAILURE, 0, "Wrong keymap number");

	if (lk_add_map(ctx, 1) != 0)
		error(EXIT_FAILURE, 0, "Unable to define map");

	lk_get_kmapinfo(ctx, &info);

	if (info.keymaps != 2)
		error(EXIT_FAILURE, 0, "Wrong keymap number");

	if (lk_add_map(ctx, 2) != 0)
		error(EXIT_FAILURE, 0, "Unable to define map");

	lk_get_kmapinfo(ctx, &info);

	if (info.keymaps != 3)
		error(EXIT_FAILURE, 0, "Wrong keymap number");

	lk_free(ctx);

	return EXIT_SUCCESS;
}
