#ifndef _FINDFILE_H
#define _FINDFILE_H

#include <sys/param.h>

typedef struct lkfile {
	FILE *fd;
	int pipe;
	char pathname[MAXPATHLEN];
} lkfile_t;

extern char pathname[];
extern int ispipe;

extern void fpclose(FILE *fp);
extern void fpclose1(lkfile_t *fp);
extern FILE *findfile(char *fnam, char **dirpath, char **suffixes);

#endif /* _FINDFILE_H */
