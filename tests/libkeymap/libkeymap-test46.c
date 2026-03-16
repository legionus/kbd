#include <stdio.h>
#include <stdlib.h>

#include <keymap.h>
#include "libcommon.h"

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct lk_ctx *ctx;

	ctx = lk_init();
	if (!ctx)
		kbd_error(EXIT_FAILURE, 0, "Unable to initialize structure by valid pointer");

	lk_set_log_fn(ctx, NULL, NULL);

	if (lk_get_keys_total(ctx, 0) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unexpected key count for missing keymap");

	if (lk_add_map(ctx, 8) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to preallocate alt table");

	if (lk_set_keywords(ctx, LK_KEYWORD_ALTISMETA) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to enable alt-is-meta");

	if (lk_add_key(ctx, 0, 5, K(KT_LATIN, 'd')) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to add plain key binding");

	if (lk_get_keys_total(ctx, 0) != 6)
		kbd_error(EXIT_FAILURE, 0, "Unexpected key count for populated keymap");

	if (!lk_key_exists(ctx, 0, 5))
		kbd_error(EXIT_FAILURE, 0, "Missing plain key binding");

	if (!lk_key_exists(ctx, 8, 5))
		kbd_error(EXIT_FAILURE, 0, "Missing synthesized alt-is-meta binding");

	if (lk_get_key(ctx, 8, 5) != K(KT_META, 'd'))
		kbd_error(EXIT_FAILURE, 0, "Unexpected synthesized alt-is-meta binding");

	if (lk_del_key(ctx, 0, 5) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to delete key binding");

	if (lk_key_exists(ctx, 0, 5))
		kbd_error(EXIT_FAILURE, 0, "Deleted key binding still exists");

	if (lk_get_key(ctx, 0, 5) != K_HOLE)
		kbd_error(EXIT_FAILURE, 0, "Deleted key binding did not become a hole");

	if (lk_set_keywords(ctx, LK_KEYWORD_KEYMAPS) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to enable explicit keymaps");

	if (lk_add_key(ctx, 3, 1, K(KT_LATIN, 'x')) == 0)
		kbd_error(EXIT_FAILURE, 0, "Explicit keymaps unexpectedly allowed implicit map creation");

	if (lk_get_key(ctx, 99, 0) != -1)
		kbd_error(EXIT_FAILURE, 0, "Missing keymap did not report failure");

	lk_free(ctx);

	return EXIT_SUCCESS;
}
