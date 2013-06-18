#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <keymap.h>

int main(int argc, char **argv)
{
	char table_shape, numeric;
	struct lk_ctx ctx;
	lkfile_t f;

	if (argc == 1) {
		printf("Usage: %s <keymap> <table_shape> <numeric>\n", argv[0]);
		return 1;
	}

	if      (!strcasecmp(argv[2], "FULL_TABLE"))     table_shape = FULL_TABLE;
	else if (!strcasecmp(argv[2], "SEPARATE_LINES")) table_shape = SEPARATE_LINES;
	else if (!strcasecmp(argv[2], "UNTIL_HOLE"))     table_shape = UNTIL_HOLE;
	else                                             table_shape = DEFAULT;

	numeric = (!strcasecmp(argv[3], "TRUE")) ? 1 : 0;

	lk_init(&ctx);
	ctx.flags |= LK_FLAG_PREFER_UNICODE;

	f.pipe = 0;
	strcpy(f.pathname, argv[1]);
	f.fd = fopen( argv[1], "r");

	lk_parse_keymap(&ctx, &f);
	lk_dump_keymap(&ctx, stdout, table_shape, numeric);
	lk_dump_diacs(&ctx, stdout);

	lk_free(&ctx);
	return 0;
}
