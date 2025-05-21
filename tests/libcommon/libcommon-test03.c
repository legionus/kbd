#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "libcommon.h"

int main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	int console;
	char *name;

	if (!isatty(0))
		return EXIT_SUCCESS;

	console = getfd(NULL);
	name = ttyname(console);
	close(console);

	console = getfd(name);
	close(console);

	return EXIT_SUCCESS;
}
