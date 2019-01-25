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

	const char *const dirpath[]  = { "", DATADIR "/findfile/test_0/keymaps/**", 0 };
	const char *const suffixes[] = { ".map", 0 };

	const char *expect = DATADIR "/findfile/test_0/keymaps/i386/qwerty/test3.map";

	int rc = 0;

	rc = kbdfile_find((char *)"i386/qwerty/test3", (char **) dirpath, (char **) suffixes, fp);

	if (rc != 0)
		error(EXIT_FAILURE, 0, "unable to find file");

	if (strcmp(expect, kbdfile_get_pathname(fp)) != 0)
		error(EXIT_FAILURE, 0, "unexpected file: %s (expected %s)", kbdfile_get_pathname(fp), expect);

	kbdfile_free(fp);
	kbdfile_context_free(ctx);

	return EXIT_SUCCESS;
}
