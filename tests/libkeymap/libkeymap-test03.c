#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <keymap.h>
#include "libcommon.h"

static void KBD_ATTR_PRINTF(6, 0)
log_null(void *data KBD_ATTR_UNUSED,
         int priority KBD_ATTR_UNUSED,
         const char *file KBD_ATTR_UNUSED,
         const int line KBD_ATTR_UNUSED,
         const char *fn KBD_ATTR_UNUSED,
         const char *format KBD_ATTR_UNUSED,
	 va_list args KBD_ATTR_UNUSED)
{
	return;
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct lk_ctx *ctx;

	if ((ctx = lk_init()) == NULL)
		kbd_error(EXIT_FAILURE, 0, "Unable to initialize structure by valid pointer");

	lk_set_log_priority(ctx, LOG_ERR);

	if (lk_get_log_priority(ctx) != LOG_ERR)
		kbd_error(EXIT_FAILURE, 0, "Unable to set log priority");

	lk_set_parser_flags(ctx, LK_FLAG_PREFER_UNICODE);

	if (lk_get_parser_flags(ctx) != LK_FLAG_PREFER_UNICODE)
		kbd_error(EXIT_FAILURE, 0, "Unable to set parser flags");

	lk_set_keywords(ctx, LK_KEYWORD_ALTISMETA | LK_KEYWORD_CHARSET);

	if (lk_get_keywords(ctx) != (LK_KEYWORD_ALTISMETA | LK_KEYWORD_CHARSET))
		kbd_error(EXIT_FAILURE, 0, "Unable to set keywords");

	const char *data = "abc";

	lk_set_log_fn(ctx, log_null, data);

	if (lk_get_log_fn(ctx) != log_null)
		kbd_error(EXIT_FAILURE, 0, "Unable to set log handler");

	if (lk_get_log_data(ctx) != data)
		kbd_error(EXIT_FAILURE, 0, "Unable to set log data");

	lk_free(ctx);

	return EXIT_SUCCESS;
}
