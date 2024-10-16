#ifndef _LIBCOMMON_H_
#define _LIBCOMMON_H_

#include <errno.h>
#include <kbd/compiler_attributes.h>

#include "nls.h"

struct kbd_help {
	const char *opts;
	const char *desc;
};

// getfd.c
int getfd(const char *fnam);

// version.c
void print_version_and_exit(void)
	KBD_ATTR_NORETURN;

void print_options(const struct kbd_help *options);
void print_report_bugs(void);

// error.c
void kbd_warning(const int errnum, const char *fmt, ...)
	KBD_ATTR_PRINTF(2, 3);

void
kbd_error(const int exitnum, const int errnum, const char *fmt, ...)
	KBD_ATTR_PRINTF(3, 4)
	KBD_ATTR_NORETURN;

#endif /* _LIBCOMMON_H_ */
