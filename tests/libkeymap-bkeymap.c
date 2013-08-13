#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <keymap.h>

int main(int argc, char **argv)
{
	struct lk_ctx *ctx;
	lkfile_t f;

	ctx = lk_init();

	f.pipe = 0;
	strcpy(f.pathname, argv[1]);
	f.fd = fopen( argv[1], "r");

	lk_parse_keymap(ctx, &f);
	lk_dump_bkeymap(ctx, stdout);

	lk_free(ctx);
	return 0;
}
