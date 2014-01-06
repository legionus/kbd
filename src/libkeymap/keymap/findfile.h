/**
 * @file findfile.h
 * @brief Functions for search, open and close a file objects.
 */
#ifndef LK_FINDFILE_H
#define LK_FINDFILE_H

#include <stdio.h>
#include <sys/param.h>

/**
 * @brief Structure contains information about an open keymap file.
 */
typedef struct lkfile {
	FILE *fd;
	int pipe;
	char pathname[MAXPATHLEN];
} lkfile_t;

lkfile_t *lk_fpopen(const char *filename);
void lk_fpclose(lkfile_t *fp);
int lk_findfile(const char *fnam, const char *const *dirpath, const char *const *suffixes, lkfile_t *fp);

#endif /* LK_FINDFILE_H */
