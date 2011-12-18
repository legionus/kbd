#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include "xmalloc.h"
#include "findfile.h"
#include "nls.h"

char pathname[MAXPATHLEN];
int ispipe;

void fpclose(FILE *fp) {
	if (ispipe)
	     pclose(fp);
	else
	     fclose(fp);
}

void fpclose1(lkfile_t *fp) {
	if (fp->pipe)
		pclose(fp->fd);
	else
		fclose(fp->fd);
}

#define SIZE(a) (sizeof(a)/sizeof(a[0]))

static struct decompressor {
	char *ext;		/* starts with `.', has no other dots */
	char *cmd;
} decompressors[] = {
	{ ".gz", "gzip -d -c" },
	{ ".bz2", "bzip2 -d -c" },
	{ 0, 0 }
};

static FILE *
pipe_open(const char *filename, struct decompressor *dc) {
	char *pipe_cmd;
	FILE *fp;

	ispipe = 1;
	pipe_cmd = xmalloc(strlen(dc->cmd) + strlen(filename) + 2);
	sprintf(pipe_cmd, "%s %s", dc->cmd, filename);
	fp = popen(pipe_cmd, "r");
	if (fp == NULL)
		fprintf(stderr, _("error executing  %s\n"), pipe_cmd);
	xfree(pipe_cmd);
	return fp;
}

/* If a file PATHNAME exists, then open it.
   If is has a `compressed' extension, then open a pipe reading it */
static FILE *
maybe_pipe_open(const char *filename) {
	FILE *fp;
	char *t;
	struct decompressor *dc;

	if ((fp = fopen(filename, "r")) != NULL) {
	    t = strrchr(filename, '.');
	    if (t) {
		for (dc = &decompressors[0]; dc->cmd; dc++)
		    if (strcmp(t, dc->ext) == 0) {
			fclose(fp);
			return pipe_open(filename, dc);
		    }
	    }
	}
	return fp;
}

static FILE *
findfile_by_fullname(const char *fnam, char **suffixes,
                     char *pathbuf, size_t bufsz)
{
	FILE *fp = NULL;
	char **sp;
	struct stat st;
	struct decompressor *dc;
	size_t fnam_len, sp_len;

	fnam_len = strlen(fnam);

	for (sp = suffixes; *sp; sp++) {
		if (*sp == 0)
			continue; /* we tried it already */

		sp_len = strlen(*sp);

		if (fnam_len + sp_len + 1 > bufsz)
			continue;

		sprintf(pathbuf, "%s%s", fnam, *sp);

		if(stat(pathbuf, &st) == 0
		   && S_ISREG(st.st_mode)
		   && (fp = fopen(pathbuf, "r")) != NULL)
			return fp;

		for (dc = &decompressors[0]; dc->cmd; dc++) {
			if (fnam_len + sp_len + strlen(dc->ext) + 1 > bufsz)
				continue;

			sprintf(pathbuf, "%s%s%s", fnam, *sp, dc->ext);

			if (stat(pathbuf, &st) == 0
			    && S_ISREG(st.st_mode)
			    && access(pathbuf, R_OK) == 0)
				return pipe_open(pathbuf, dc);
		}
	}

	return NULL;
}

static FILE *
findfile_in_dir(char *fnam, char *dir, int recdepth, char **suf,
                char *pathbuf, size_t bufsz)
{
	FILE *fp = NULL;
	DIR *d;
	struct dirent *de;
	char *ff, *fdir, *p, *q, **sp;
	struct decompressor *dc;
	int secondpass = 0;
	size_t dir_len;

	ispipe = 0;

	if ((d = opendir(dir)) == NULL)
	    return NULL;

	dir_len = strlen(dir);

	fdir = NULL;
	if ((ff = strchr(fnam, '/')) != NULL)
		fdir = xstrndup(fnam, ff - fnam);

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

	    if (dir_len + d_len + 2 > bufsz) {
		fprintf(stderr, _("Warning: path too long: %s/%s\n"), dir, de->d_name);
		continue;
	    }

	    okdir = (ff && strcmp(de->d_name, fdir) == 0);

	    if ((secondpass && recdepth) || okdir) {
		char *a;

		a = xmalloc(dir_len + d_len + 2);
		sprintf(a, "%s/%s", dir, de->d_name);

		if (stat(a, &st) == 0 && S_ISDIR(st.st_mode)) {
			if (okdir)
				fp = findfile_in_dir(ff+1, a, 0, suf,
				                     pathbuf, bufsz);

			if (!fp && recdepth)
				fp = findfile_in_dir(fnam, a, recdepth-1, suf,
				                     pathbuf, bufsz);

			if (fp) {
				xfree(a);
				goto EndScan;
			}
		}
		xfree(a);
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

	    sprintf(pathbuf, "%s/%s", dir, de->d_name);
	    if (stat(pathbuf, &st) != 0 || !S_ISREG(st.st_mode))
		    continue;

	    /* Does tail consist of a known suffix and possibly
	       a compression suffix? */
	    for(sp = suf; *sp; sp++) {
		    size_t l;

		    if (!strcmp(p, *sp)) {
	    		fp = maybe_pipe_open(pathbuf);
	    		goto EndScan;
		    }

		    l = strlen(*sp);
		    if (!strncmp(p, *sp, l)) {
			for (dc = &decompressors[0]; dc->cmd; dc++)
			    if (strcmp(p+l, dc->ext) == 0) {
			    	fp = pipe_open(pathbuf, dc);
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
	xfree(fdir);
	closedir(d);
	return fp;
}

/* find input file; leave name in pathname[] */
FILE *
findfile(char *fnam, char **dirpath, char **suffixes) {
	char **dp, *dir;
	FILE *fp = NULL;
	int dl, recdepth;

	/* Try explicitly given name first */
	strcpy(pathname, fnam);
	fp = maybe_pipe_open(fnam);
	if (fp)
		return fp;

	/* Test for full pathname - opening it failed, so need suffix */
	/* (This is just nonsense, for backwards compatibility.) */
	if (*fnam == '/')
		return findfile_by_fullname(fnam, suffixes,
		                            pathname, sizeof(pathname));

	/* Search a list of directories and directory hierarchies */
	for (dp = dirpath; (*dp && !fp); dp++) {
		recdepth = 0;
		dl = strlen(*dp);

		/* trailing stars denote recursion */
		while (dl && (*dp)[dl-1] == '*')
			dl--, recdepth++;

		/* delete trailing slashes */
		while (dl && (*dp)[dl-1] == '/')
			dl--;

		if (dl)
			dir = xstrndup(*dp, dl);
		else
			dir = xstrdup(".");

		fp = findfile_in_dir(fnam, dir, recdepth, suffixes,
		                     pathname, sizeof(pathname));
		xfree(dir);
	}
	return fp;
}
