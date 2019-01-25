#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <error.h>
#include <errno.h>

#include <kbdfile.h>

int
main(void)
{
	struct kbdfile_ctx *ctx = kbdfile_context_new();
	if (!ctx)
		error(EXIT_FAILURE, 0, "unable to create context");

	struct kbdfile *fp = kbdfile_new(ctx);
	if (!fp)
		error(EXIT_FAILURE, 0, "unable to create kbdfile");

	kbdfile_free(fp);
	kbdfile_context_free(ctx);

	return EXIT_SUCCESS;
}
