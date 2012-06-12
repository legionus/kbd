#ifndef _FINDFILE_H
#define _FINDFILE_H

#include <sys/param.h>

typedef struct lkfile {
	FILE *fd;
	int pipe;
	char pathname[MAXPATHLEN];
} lkfile_t;

extern void fpclose(lkfile_t *fp);
extern int findfile(char *fnam, char **dirpath, char **suffixes, lkfile_t *fp);

#endif /* _FINDFILE_H */
