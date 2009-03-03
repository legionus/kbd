#ifndef _FINDFILE_H
#define _FINDFILE_H

extern void fpclose(FILE *fp);
extern FILE *findfile(char *fnam, char **dirpath, char **suffixes);

#endif /* _FINDFILE_H */
