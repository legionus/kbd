#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "libkbdfile-test.h"

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	open_and_expect_kbdfile(TESTDIR "/data/findfile/test_0/keymaps/i386/qwerty/test0.map",
			TESTDIR "/data/findfile/test_0/keymaps/i386/qwerty/test0.map");

	return EXIT_SUCCESS;
}
