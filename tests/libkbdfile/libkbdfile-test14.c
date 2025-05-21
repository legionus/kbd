#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <err.h>

#include <kbdfile.h>
#include "libcommon.h"

#define __stringify_1(x...)	#x
#define __stringify(x...)	__stringify_1(x)

#define CHECK_RETCODE(funcname, expcode, args...) \
	do { \
		if (funcname(args) != expcode) \
			errx(EXIT_FAILURE, __stringify(funcname) ": unexpected return code"); \
	} while(0)

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct kbdfile_ctx *ctx = kbdfile_context_new();

	CHECK_RETCODE(kbdfile_get_log_fn, NULL, NULL);
	CHECK_RETCODE(kbdfile_set_log_fn, -1, NULL, NULL, NULL);
	CHECK_RETCODE(kbdfile_get_log_data, NULL, NULL);
	CHECK_RETCODE(kbdfile_set_log_data, -1, NULL, NULL);
	CHECK_RETCODE(kbdfile_get_log_priority, -1, NULL);
	CHECK_RETCODE(kbdfile_set_log_priority, -1, NULL, 0);
	CHECK_RETCODE(kbdfile_context_free, NULL, NULL);

	CHECK_RETCODE(kbdfile_set_log_priority, 0, ctx, 123);
	CHECK_RETCODE(kbdfile_get_log_priority, 123, ctx);

	const char *data = "data";

	CHECK_RETCODE(kbdfile_set_log_data, 0, ctx, data);
	CHECK_RETCODE(kbdfile_get_log_data, data, ctx);

	CHECK_RETCODE(kbdfile_context_free, NULL, ctx);

	return EXIT_SUCCESS;
}
