#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <keymap.h>
#include "libcommon.h"

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct lk_ctx *ctx;

	ctx = lk_init();
	lk_set_log_fn(ctx, NULL, NULL);

	if (lk_add_key(ctx, 0, 0, 16) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to add keycode");

	if (lk_get_key(ctx, 0, 0) != 16)
		kbd_error(EXIT_FAILURE, 0, "Unable to get keycode");

	lk_free(ctx);

	return EXIT_SUCCESS;
}
