#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <kbdfile.h>
#include "private/common.h"

int
main(int __attribute__((unused)) argc, char **argv)
{
	set_progname(argv[0]);

	struct kbdfile *fp = kbdfile_new(NULL);
	if (!fp)
		kbd_error(EXIT_FAILURE, 0, "unable to create kbdfile");

	const char *const dirpath[]  = { "", TESTDIR "/data/findfile/test_0/keymaps/**", 0 };
	const char *const suffixes[] = { ".map", 0 };

	const char *expect = TESTDIR "/data/findfile/test_0/keymaps/i386/qwerty/test3.map";

	int rc = 0;

	rc = kbdfile_find("i386/qwerty/test3", dirpath, suffixes, fp);

	if (rc != 0)
		kbd_error(EXIT_FAILURE, 0, "unable to find file");

	if (strcmp(expect, kbdfile_get_pathname(fp)) != 0)
		kbd_error(EXIT_FAILURE, 0, "unexpected file: %s (expected %s)", kbdfile_get_pathname(fp), expect);

	kbdfile_free(fp);

	return EXIT_SUCCESS;
}
