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

void fpclose(FILE *fp)
{
	if (ispipe)
	     pclose(fp);
	else
	     fclose(fp);
}

void fpclose1(lkfile_t *fp)
{
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

static int
pipe_open(const char *filename, struct decompressor *dc, lkfile_t *fp)
{
	char *pipe_cmd;

	pipe_cmd = xmalloc(strlen(dc->cmd) + strlen(filename) + 2);
	sprintf(pipe_cmd, "%s %s", dc->cmd, filename);

	fp->fd = popen(pipe_cmd, "r");
	fp->pipe = 1;

	if (fp->fd == NULL) {
		fprintf(stderr, _("error executing  %s\n"), pipe_cmd);
		xfree(pipe_cmd);
		return -1;
	}

	xfree(pipe_cmd);
	return 0;
}

/* If a file PATHNAME exists, then open it.
   If is has a `compressed' extension, then open a pipe reading it */
static int
maybe_pipe_open(const char *filename, lkfile_t *fp)
{
	char *t;
	struct stat st;
	struct decompressor *dc;

	if (stat(filename, &st) == -1 || !S_ISREG(st.st_mode) ||
	    access(filename, R_OK) == -1)
		return -1;

	t = strrchr(filename, '.');
	if (t) {
		for (dc = &decompressors[0]; dc->cmd; dc++) {
			if (strcmp(t, dc->ext) == 0)
				return pipe_open(filename, dc, fp);
		}
	}
	fp->fd = fopen(filename, "r");
	fp->pipe = 0;

	if (fp->fd == NULL)
		return -1;

	return 0;
}

static int
findfile_by_fullname(const char *fnam, char **suffixes,
                     char *pathbuf, size_t bufsz, lkfile_t *fp)
{
	char **sp;
	struct stat st;
	struct decompressor *dc;
	size_t fnam_len, sp_len;

	fp->pipe = 0;
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
		   && (fp->fd = fopen(pathbuf, "r")) != NULL)
			return 0;

		for (dc = &decompressors[0]; dc->cmd; dc++) {
			if (fnam_len + sp_len + strlen(dc->ext) + 1 > bufsz)
				continue;

			sprintf(pathbuf, "%s%s%s", fnam, *sp, dc->ext);

			if (stat(pathbuf, &st) == 0
			    && S_ISREG(st.st_mode)
			    && access(pathbuf, R_OK) == 0)
				return pipe_open(pathbuf, dc, fp);
		}
	}

	return -1;
}

static int
findfile_in_dir(char *fnam, char *dir, int recdepth, char **suf,
                char *pathbuf, size_t bufsz, lkfile_t *fp)
{
	DIR *d;
	struct dirent *de;
	char *ff, *fdir, *p, *q, **sp;
	struct decompressor *dc;
	int rc = -1, secondpass = 0;
	size_t dir_len;

	fp->fd = NULL;
	fp->pipe = 0;

	if ((d = opendir(dir)) == NULL)
		return -1;

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
				rc = findfile_in_dir(ff+1, a, 0, suf,
				                     pathbuf, bufsz, fp);

			if (rc && recdepth)
				rc = findfile_in_dir(fnam, a, recdepth-1, suf,
				                     pathbuf, bufsz, fp);

			if (!rc) {
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
	    		rc = maybe_pipe_open(pathbuf, fp);
	    		goto EndScan;
		    }

		    l = strlen(*sp);
		    if (!strncmp(p, *sp, l)) {
			for (dc = &decompressors[0]; dc->cmd; dc++)
			    if (strcmp(p+l, dc->ext) == 0) {
			    	rc = pipe_open(pathbuf, dc, fp);
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
	return rc;
}

/* find input file; leave name in pathname[] */
FILE *
findfile(char *fnam, char **dirpath, char **suffixes)
{
	char **dp, *dir;
	int dl, recdepth, rc;
	lkfile_t fp = { NULL, 0 };

	/* Try explicitly given name first */
	strcpy(pathname, fnam);

	if (!maybe_pipe_open(fnam, &fp))
		goto good;

	/* Test for full pathname - opening it failed, so need suffix */
	/* (This is just nonsense, for backwards compatibility.) */
	if (*fnam == '/' &&
	    !findfile_by_fullname(fnam, suffixes, pathname, sizeof(pathname), &fp))
		goto good;

	/* Search a list of directories and directory hierarchies */
	for (dp = dirpath; *dp; dp++) {
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

		rc = findfile_in_dir(fnam, dir, recdepth, suffixes,
		                     pathname, sizeof(pathname), &fp);
		xfree(dir);

		if (!rc)
			goto good;
	}
	return NULL;
good:
	ispipe = fp.pipe;
	return fp.fd;
}
