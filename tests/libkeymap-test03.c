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

	if ((ctx = lk_init()) == NULL)
		error(EXIT_FAILURE, 0, "Unable to initialize structure by valid pointer");

	lk_free(ctx);

	return EXIT_SUCCESS;
}
