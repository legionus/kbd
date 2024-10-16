#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <kbdfile.h>
#include "libcommon.h"

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct kbdfile *fp = kbdfile_open(NULL, TESTDIR "/data/findfile/test_0/keymaps/i386/qwerty/test0");
	if (fp)
		kbd_error(EXIT_FAILURE, 0, "unexpected kbdfile");

	kbdfile_free(fp);

	return EXIT_SUCCESS;
}
