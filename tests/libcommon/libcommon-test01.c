#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "libcommon.h"

int main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	const struct kbd_help opthelp[] = {
		{ "-C, --console=DEV", "the console device to be used." },
		{ "-V, --version",     "print version number."     },
		{ "-h, --help",        "print this usage message." },
		{ NULL, NULL }
	};

	print_options(opthelp);

	return EXIT_SUCCESS;
}
