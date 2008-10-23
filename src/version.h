#include <string.h>
#include <stdlib.h>

#include "../config.h"

char *progname;

static inline void
set_progname(char *name) {
	char *p;

	p = rindex(name, '/');
	progname = (p ? p+1 : name);
}

static inline void
print_version_and_exit(void) {
	printf(_("%s from %s\n"), progname, PACKAGE_STRING);
	exit(0);
}
