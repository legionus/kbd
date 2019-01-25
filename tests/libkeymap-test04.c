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

	if (ctx == NULL)
		error(EXIT_FAILURE, 0, "Unable to initialize structure by valid pointer");

	if (lk_free(ctx) != 0)
		error(EXIT_FAILURE, 0, "Unable to free by valid pointer");

	if (lk_free(NULL) == 0)
		error(EXIT_FAILURE, 0, "Possible to free NULL pointer");

	return EXIT_SUCCESS;
}
