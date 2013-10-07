#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <keymap.h>

#include "contextP.h"

int main(int argc, char **argv)
{
	int i;
	struct lk_ctx *ctx;
	lkfile_t f;

	ctx = lk_init();

	f.pipe = 0;
	strcpy(f.pathname, argv[1]);
	f.fd = fopen( argv[1], "r");

	lk_parse_keymap(ctx, &f);

	for (i = 0; i < ctx->keymap->total; i++) {
		if (!lk_map_exists(ctx, i))
			continue;
		printf("keymap %03d\n", i);
	}

	lk_free(ctx);
	return 0;
}
