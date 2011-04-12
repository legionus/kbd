#ifndef _FINDFILE_H
#define _FINDFILE_H

typedef struct lkfile {
	FILE *fd;
	int pipe;
} lkfile_y;

extern char pathname[];
extern int ispipe;

extern void fpclose(FILE *fp);
extern void fpclose1(FILE *fp, int pipe);
//extern void fpclose1(lkfile_t *fp);
extern FILE *findfile(char *fnam, char **dirpath, char **suffixes);

#endif /* _FINDFILE_H */
