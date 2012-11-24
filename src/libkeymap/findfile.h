#ifndef _FINDFILE_H
#define _FINDFILE_H

#include <sys/param.h>

typedef struct lkfile {
	FILE *fd;
	int pipe;
	char pathname[MAXPATHLEN];
} lkfile_t;

extern void fpclose(lkfile_t *fp);
extern int findfile(const char *fnam, const char *const *dirpath, const char *const *suffixes, lkfile_t *fp);

#endif /* _FINDFILE_H */
