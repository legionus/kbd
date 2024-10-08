#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <keymap.h>
#include "libcommon.h"

int
main(int argc KBD_ATTR_UNUSED, char **argv)
{
	set_progname(argv[0]);

	struct lk_ctx *ctx;

	if ((ctx = lk_init()) == NULL)
		kbd_error(EXIT_FAILURE, 0, "Unable to initialize structure by valid pointer");

	lk_free(ctx);

	return EXIT_SUCCESS;
}
