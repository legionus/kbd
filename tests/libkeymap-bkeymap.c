#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <keymap.h>

int main(int argc, char **argv)
{
	struct keymap kmap;
	lkfile_t f;

	lk_init(&kmap);

	f.pipe = 0;
	strcpy(f.pathname, argv[1]);
	f.fd = fopen( argv[1], "r");

	lk_parse_keymap(&kmap, &f);
	lk_dump_bkeymap(&kmap, stdout);

	lk_free(&kmap);
	return 0;
}
