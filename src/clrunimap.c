/*
 * clrunimap.c
 *
 * Note: nowadays this kills kernel console output!
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <linux/kd.h>

#include <kfont.h>

#include "libcommon.h"

int main(int argc, char *argv[])
{
	int fd;
	char *console = NULL;

	setuplocale();

	if (argc >= 3 && !strcmp(argv[1], "-C"))
		console = argv[2];

	if ((fd = getfd(console)) < 0)
		kbd_error(EXIT_FAILURE, 0, _("Couldn't get a file descriptor referring to the console."));

	int ret;
	struct kfont_context *ctx;

	if ((ret = kfont_init(program_invocation_short_name, &ctx)) < 0)
		return -ret;

	return kfont_put_unicodemap(ctx, fd, NULL, NULL);
}
