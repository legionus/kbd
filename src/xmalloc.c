/* Error-free versions of some libc routines */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include "kbd.h"
#include "nls.h"
#include "xmalloc.h"

extern char *progname;

static void __attribute__ ((noreturn))
nomem(void) {
	fprintf(stderr, _("%s: out of memory\n"), progname);
	exit(EX_OSERR);
}

void *
xmalloc(size_t sz) {
	void *p = malloc(sz);
	if (p == NULL)
		nomem();
	return p;
}

void *
xrealloc(void *pp, size_t sz) {
	void *p = realloc(pp, sz);
	if (p == NULL)
		nomem();
	return p;
}

char *
xstrdup(char *p) {
	char *q = strdup(p);
	if (q == NULL)
		nomem();
	return q;
}

char *
xstrndup(char *p, size_t n) {
	char *q = strndup(p, n);
	if (q == NULL)
		nomem();
	return q;
}

void *
xfree(void *p) {
	if (p != NULL)
		free(p);
	return NULL;
}
