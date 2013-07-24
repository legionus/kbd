#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <keymap.h>

int main(int argc, char **argv)
{
	lk_table_shape table;
	char numeric;
	struct lk_ctx *ctx;
	lkfile_t f;

	if (argc == 1) {
		printf("Usage: %s <keymap> <table_shape> <numeric>\n", argv[0]);
		return 1;
	}

	if      (!strcasecmp(argv[2], "FULL_TABLE"))     table = LK_SHAPE_FULL_TABLE;
	else if (!strcasecmp(argv[2], "SEPARATE_LINES")) table = LK_SHAPE_SEPARATE_LINES;
	else if (!strcasecmp(argv[2], "UNTIL_HOLE"))     table = LK_SHAPE_UNTIL_HOLE;
	else                                             table = LK_SHAPE_DEFAULT;

	numeric = (!strcasecmp(argv[3], "TRUE")) ? 1 : 0;

	ctx = lk_init();
	lk_set_parser_flags(ctx, LK_FLAG_PREFER_UNICODE);

	f.pipe = 0;
	strcpy(f.pathname, argv[1]);
	f.fd = fopen( argv[1], "r");

	lk_parse_keymap(ctx, &f);
	lk_dump_keymap(ctx, stdout, table, numeric);
	lk_dump_diacs(ctx, stdout);

	lk_free(ctx);
	return 0;
}
