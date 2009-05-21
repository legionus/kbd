#include "kbd.h"
#include "../config.h"
#ifdef __klibc__
#include "klibc_compat.h"
#endif
#include <string.h>
#include <stdlib.h>

char *progname;

static inline void
set_progname(char *name) {
	char *p;

	p = rindex(name, '/');
	progname = (p ? p+1 : name);
}

static inline void attr_noreturn
print_version_and_exit(void) {
	printf(_("%s from %s\n"), progname, PACKAGE_STRING);
	exit(0);
}
