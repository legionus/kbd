#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "libkbdfile-test.h"

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct kbdfile *fp = new_test_kbdfile();

	const char *const dirpath[]  = { "", TESTDIR "/data/findfile/test_1/consolefonts/", NULL };
	const char *const suffixes[] = { "", ".psfu", ".psf", ".cp", ".fnt", NULL };

	const char *expect = TESTDIR "/data/findfile/test_1/consolefonts/simple-1.psf.gz";

	find_and_expect_kbdfile(fp, "simple-1.psf", dirpath, suffixes, expect);

	if (!kbdfile_is_compressed(fp))
		kbd_error(EXIT_FAILURE, 0, "not compressed: %s\n", kbdfile_get_pathname(fp));

	kbdfile_free(fp);

	return EXIT_SUCCESS;
}
