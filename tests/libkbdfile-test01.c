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
	kbdfile_free(fp);
	return EXIT_SUCCESS;
}
