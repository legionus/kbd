#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include "kbd.h"
#include "kbd_error.h"

extern const char *progname;

void
    __attribute__((format(printf, 2, 3)))
    kbd_warning(const int errnum, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);

	fprintf(stderr, "%s: ", progname);
	vfprintf(stderr, fmt, ap);

	if (errnum > 0)
		fprintf(stderr, ": %s\n", strerror(errnum));

	va_end(ap);
	return;
}

void
    __attribute__((noreturn))
    __attribute__((format(printf, 3, 4)))
    kbd_error(const int exitnum, const int errnum, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	fprintf(stderr, "%s: ", progname);
	vfprintf(stderr, fmt, ap);

	va_end(ap);

	if (errnum > 0)
		fprintf(stderr, ": %s\n", strerror(errnum));

	exit(exitnum);
}
