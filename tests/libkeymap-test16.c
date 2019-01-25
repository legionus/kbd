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

	ctx = lk_init();
	lk_set_log_fn(ctx, NULL, NULL);
	lk_set_parser_flags(ctx, LK_KEYWORD_ALTISMETA);

	if (lk_add_key(ctx, 0, 0, 16) != 0)
		error(EXIT_FAILURE, 0, "Unable to add keycode");

	if (lk_get_key(ctx, 0, 0) != 16)
		error(EXIT_FAILURE, 0, "Unable to get keycode");

	lk_free(ctx);

	return EXIT_SUCCESS;
}
