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

	struct kbdfile *fp = kbdfile_open(ctx, DATADIR "/findfile/test_0/keymaps/i386/qwerty/test0");
	if (fp)
		error(EXIT_FAILURE, 0, "unexpected kbdfile");

	kbdfile_free(fp);
	kbdfile_context_free(ctx);

	return EXIT_SUCCESS;
}
