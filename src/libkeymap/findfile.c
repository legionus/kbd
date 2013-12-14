#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include "nls.h"
#include "keymap/findfile.h"

void
lk_fpclose(lkfile_t *fp)
{
	if (!fp || !fp->fd)
		return;
	if (fp->pipe)
		pclose(fp->fd);
	else
		fclose(fp->fd);
	fp->fd = NULL;
}

#define SIZE(a) (sizeof(a)/sizeof(a[0]))

static struct decompressor {
	const char *ext; /* starts with `.', has no other dots */
	const char *cmd;
} decompressors[] = {
	{ ".gz", "gzip -d -c" },
	{ ".bz2", "bzip2 -d -c" },
	{ 0, 0 }
};

static int
pipe_open(const struct decompressor *dc, lkfile_t *fp)
{
	char *pipe_cmd;

	pipe_cmd = malloc(strlen(dc->cmd) + strlen(fp->pathname) + 2);
	if (pipe_cmd == NULL)
		return -1;

	sprintf(pipe_cmd, "%s %s", dc->cmd, fp->pathname);

	fp->fd = popen(pipe_cmd, "r");
	fp->pipe = 1;

	free(pipe_cmd);

	if (fp->fd == NULL)
		return -1;
	return 0;
}

/* If a file PATHNAME exists, then open it.
   If is has a `compressed' extension, then open a pipe reading it */
static int
maybe_pipe_open(lkfile_t *fp)
{
	char *t;
	struct stat st;
	struct decompressor *dc;

	if (stat(fp->pathname, &st) == -1 || !S_ISREG(st.st_mode) ||
	    access(fp->pathname, R_OK) == -1)
		return -1;

	t = strrchr(fp->pathname, '.');
	if (t) {
		for (dc = &decompressors[0]; dc->cmd; dc++) {
			if (strcmp(t, dc->ext) == 0)
				return pipe_open(dc, fp);
		}
	}
	fp->fd = fopen(fp->pathname, "r");
	fp->pipe = 0;

	if (fp->fd == NULL)
		return -1;

	return 0;
}

static int
findfile_by_fullname(const char *fnam, const char *const *suffixes, lkfile_t *fp)
{
	int i;
	struct stat st;
	struct decompressor *dc;
	size_t fnam_len, sp_len;

	fp->pipe = 0;
	fnam_len = strlen(fnam);

	for (i = 0; suffixes[i]; i++) {
		if (suffixes[i] == 0)
			continue; /* we tried it already */

		sp_len = strlen(suffixes[i]);

		if (fnam_len + sp_len + 1 > sizeof(fp->pathname))
			continue;

		sprintf(fp->pathname, "%s%s", fnam, suffixes[i]);

		if(stat(fp->pathname, &st) == 0
		   && S_ISREG(st.st_mode)
		   && (fp->fd = fopen(fp->pathname, "r")) != NULL)
			return 0;

		for (dc = &decompressors[0]; dc->cmd; dc++) {
			if (fnam_len + sp_len + strlen(dc->ext) + 1 > sizeof(fp->pathname))
				continue;

			sprintf(fp->pathname, "%s%s%s", fnam, suffixes[i], dc->ext);

			if (stat(fp->pathname, &st) == 0
			    && S_ISREG(st.st_mode)
			    && access(fp->pathname, R_OK) == 0)
				return pipe_open(dc, fp);
		}
	}

	return -1;
}

static int
findfile_in_dir(const char *fnam, const char *dir, const int recdepth, const char *const *suf, lkfile_t *fp)
{
	DIR *d;
	struct dirent *de;
	char *ff, *fdir, *p;
	const char *q;
	struct decompressor *dc;
	int i, rc = -1, secondpass = 0;
	size_t dir_len;

	fp->fd = NULL;
	fp->pipe = 0;

	if ((d = opendir(dir)) == NULL)
		return -1;

	dir_len = strlen(dir);

	fdir = NULL;
	if ((ff = strchr(fnam, '/')) != NULL) {
		if ((fdir = strndup(fnam, ff - fnam)) == NULL) {
			closedir(d);
			return -1;
		}
	}

	/* Scan the directory twice: first for files, then
	   for subdirectories, so that we do never search
	   a subdirectory when the directory itself already
	   contains the file we are looking for. */
StartScan:
	while ((de = readdir(d)) != NULL) {
	    struct stat st;
	    int okdir;
	    size_t d_len;

	    d_len = strlen(de->d_name);
	    if (d_len < 3) {
		    if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
			continue;
	    }

	    if (dir_len + d_len + 2 > sizeof(fp->pathname))
		continue;

	    okdir = (ff && strcmp(de->d_name, fdir) == 0);

	    if ((secondpass && recdepth) || okdir) {
		char *a;

		if ((a = malloc(dir_len + d_len + 2)) == NULL)
			goto EndScan;

		sprintf(a, "%s/%s", dir, de->d_name);

		if (stat(a, &st) == 0 && S_ISDIR(st.st_mode)) {
			if (okdir)
				rc = findfile_in_dir(ff+1, a, 0, suf, fp);

			if (rc && recdepth)
				rc = findfile_in_dir(fnam, a, recdepth-1, suf, fp);

			if (!rc) {
				free(a);
				goto EndScan;
			}
		}
		free(a);
	    }

	    if (secondpass)
		    continue;

	    /* Should we be in a subdirectory? */
	    if (ff)
		    continue;

	    /* Does d_name start right? */
	    p = &de->d_name[0];
	    q = fnam;
	    while (*p && *p == *q) p++,q++;
	    if (*q)
		    continue;

	    sprintf(fp->pathname, "%s/%s", dir, de->d_name);
	    if (stat(fp->pathname, &st) != 0 || !S_ISREG(st.st_mode))
		    continue;

	    /* Does tail consist of a known suffix and possibly
	       a compression suffix? */
	    for(i = 0; suf[i]; i++) {
		    size_t l;

		    if (!strcmp(p, suf[i])) {
	    		rc = maybe_pipe_open(fp);
	    		goto EndScan;
		    }

		    l = strlen(suf[i]);
		    if (!strncmp(p, suf[i], l)) {
			for (dc = &decompressors[0]; dc->cmd; dc++)
			    if (strcmp(p+l, dc->ext) == 0) {
			    	rc = pipe_open(dc, fp);
			    	goto EndScan;
			    }
		    }
	    }
	}

	if (recdepth > 0 && !secondpass) {
		secondpass = 1;
		seekdir(d, 0);
		goto StartScan;
	}

EndScan:
	if (fdir != NULL)
		free(fdir);
	closedir(d);
	return rc;
}

int
lk_findfile(const char *fnam, const char *const *dirpath, const char *const *suffixes, lkfile_t *fp)
{
	char *dir;
	int dl, recdepth, rc, i;

	fp->fd = NULL;
	fp->pipe = 0;

	/* Try explicitly given name first */
	strcpy(fp->pathname, fnam);

	if (!maybe_pipe_open(fp))
		return 0;

	/* Test for full pathname - opening it failed, so need suffix */
	/* (This is just nonsense, for backwards compatibility.) */
	if (*fnam == '/' &&
	    !findfile_by_fullname(fnam, suffixes, fp))
		return 0;

	/* Search a list of directories and directory hierarchies */
	for (i = 0; dirpath[i]; i++) {
		recdepth = 0;
		dl = strlen(dirpath[i]);

		/* trailing stars denote recursion */
		while (dl && dirpath[i][dl-1] == '*')
			dl--, recdepth++;

		/* delete trailing slashes */
		while (dl && dirpath[i][dl-1] == '/')
			dl--;

		if (dl)
			dir = strndup(dirpath[i], dl);
		else
			dir = strdup(".");

		if (dir == NULL)
			return 1;

		rc = findfile_in_dir(fnam, dir, recdepth, suffixes, fp);
		free(dir);

		if (!rc)
			return 0;
	}
	return 1;
}

lkfile_t *
lk_fpopen(const char *filename)
{
	lkfile_t *fp;

	fp = malloc(sizeof(lkfile_t));
	if (!fp)
		return NULL;

	strcpy(fp->pathname, filename);

	if (maybe_pipe_open(fp) < 0) {
		free(fp);
		return NULL;
	}

	return fp;
}
