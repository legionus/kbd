#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "libcommon.h"

int main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	print_report_bugs();

	return EXIT_SUCCESS;
}
