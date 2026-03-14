#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "libkbdfile-test.h"

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct kbdfile *fp = new_test_kbdfile();

	const char *const dirpath[]  = { "", TESTDIR "/data/findfile/test_0/keymaps/**", NULL };
	const char *const suffixes[] = { ".kmap", ".map", "", NULL };

	const char *expect = TESTDIR "/data/findfile/test_0/keymaps/i386/qwertz/test2.kmap";

	find_and_expect_kbdfile(fp, "test2", dirpath, suffixes, expect);

	kbdfile_free(fp);

	return EXIT_SUCCESS;
}
