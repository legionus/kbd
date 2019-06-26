#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <error.h>
#include <errno.h>

#include <kbdfile.h>

int
main(void)
{
	struct kbdfile *fp = kbdfile_open(NULL, DATADIR "/findfile/test_0/keymaps/i386/qwerty/test0");
	if (fp)
		error(EXIT_FAILURE, 0, "unexpected kbdfile");

	kbdfile_free(fp);

	return EXIT_SUCCESS;
}
