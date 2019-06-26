#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <error.h>
#include <errno.h>

#include <kbdfile.h>

int
main(void)
{
	struct kbdfile *fp = kbdfile_new(NULL);
	if (!fp)
		error(EXIT_FAILURE, 0, "unable to create kbdfile");

	const char *const dirpath[]  = { "", DATADIR "/findfile/test_0/keymaps/**", 0 };
	const char *const suffixes[] = { "", ".map", ".kmap", 0 };

	const char *expect = ABS_DATADIR "/findfile/test_0/keymaps/i386/qwerty/test0.map";

	int rc = 0;

	rc = kbdfile_find((char *)(ABS_DATADIR "/findfile/test_0/keymaps/i386/qwerty/test0"), (char **) dirpath, (char **) suffixes, fp);

	if (rc != 0)
		error(EXIT_FAILURE, 0, "unable to find file");

	if (strcmp(expect, kbdfile_get_pathname(fp)) != 0)
		error(EXIT_FAILURE, 0, "unexpected file: %s (expected %s)", kbdfile_get_pathname(fp), expect);

	kbdfile_free(fp);

	return EXIT_SUCCESS;
}
