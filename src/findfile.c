#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include "xmalloc.h"
#include "findfile.h"
#include "nls.h"

char pathname[1024];
static int ispipe;

void fpclose(FILE *fp) {
#ifndef __klibc__
	if (ispipe)
	     pclose(fp);
	else
#endif /* __klibc__ */
	     fclose(fp);
}

#define SIZE(a) (sizeof(a)/sizeof(a[0]))

#ifndef __klibc__
static struct decompressor {
	char *ext;		/* starts with `.', has no other dots */
	char *cmd;
} decompressors[] = {
	{ ".gz", "gzip -d -c" },
	{ ".bz2", "bzip2 -d -c" },
	{ 0, 0 }
};

static FILE *
pipe_open(struct decompressor *dc) {
	char *pipe_cmd;
	FILE *fp;

	ispipe = 1;
	pipe_cmd = xmalloc(strlen(dc->cmd) + strlen(pathname) + 2);
	sprintf(pipe_cmd, "%s %s", dc->cmd, pathname);
	fp = popen(pipe_cmd, "r");
	if (fp == NULL)
		fprintf(stderr, _("error executing  %s\n"), pipe_cmd);
	return fp;
}
#endif /* __klibc__ */

/* If a file PATHNAME exists, then open it.
   If is has a `compressed' extension, then open a pipe reading it */
static FILE *
maybe_pipe_open(void) {
	FILE *fp;
#ifndef __klibc__
	char *t;
	struct decompressor *dc;

	if ((fp = fopen(pathname, "r")) != NULL) {
	    t = rindex(pathname, '.');
	    if (t) {
		for (dc = &decompressors[0]; dc->cmd; dc++)
		    if (strcmp(t, dc->ext) == 0) {
			fclose(fp);
			return pipe_open(dc);
		    }
	    }
	}
#else
	fp = fopen(pathname, "r");
#endif /* __klibc__ */
	return fp;
}

static FILE *
findfile_in_dir(char *fnam, char *dir, int recdepth, char **suf) {
	FILE *fp = NULL;
	DIR *d;
	struct dirent *de;
	char *ff, *fdir, *p, *q, **sp;
#ifndef __klibc__
	struct decompressor *dc;
#endif /* __klibc__ */
	int secondpass = 0;

	ispipe = 0;

	ff = index(fnam, '/');
	if (ff) {
		fdir = xstrdup(fnam);
		fdir[ff-fnam] = 0; 	/* caller guarantees fdir != "" */
	} else
		fdir = 0;		/* just to please gcc */

	/* Scan the directory twice: first for files, then
	   for subdirectories, so that we do never search
	   a subdirectory when the directory itself already
	   contains the file we are looking for. */
 StartScan:
	d = opendir(dir);
	if (d == NULL)
	    return NULL;
	while ((de = readdir(d)) != NULL) {
	    struct stat statbuf;
	    int okdir;

	    if (strcmp(de->d_name, ".") == 0 ||
		strcmp(de->d_name, "..") == 0)
		continue;

	    if (strlen(dir) + strlen(de->d_name) + 2 > sizeof(pathname))
		continue;

	    okdir = (ff && strcmp(de->d_name, fdir) == 0);

	    if ((secondpass && recdepth) || okdir) {
		char *a;

		a = xmalloc(strlen(dir) + strlen(de->d_name) + 2);
		sprintf(a, "%s/%s", dir, de->d_name);
		if (stat(a, &statbuf) == 0 &&
		    S_ISDIR(statbuf.st_mode)) {
			if (okdir)
				fp = findfile_in_dir(ff+1, a, 0, suf);
			if (!fp && recdepth)
				fp = findfile_in_dir(fnam, a, recdepth-1, suf);
			if (fp)
				return fp;
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

	    sprintf(pathname, "%s/%s", dir, de->d_name);
	    if (stat(pathname, &statbuf) != 0 || !S_ISREG(statbuf.st_mode))
		    continue;

	    /* Does tail consist of a known suffix and possibly
	       a compression suffix? */
	    for(sp = suf; *sp; sp++) {
#ifndef __klibc__
		    int l;
#endif /* __klibc__ */

		    if (!strcmp(p, *sp))
			    return maybe_pipe_open();

#ifndef __klibc__
		    l = strlen(*sp);
		    if (strncmp(p,*sp,l) == 0) {
			for (dc = &decompressors[0]; dc->cmd; dc++)
			    if (strcmp(p+l, dc->ext) == 0)
				return pipe_open(dc);
		    }
#endif /* __klibc__ */
	    }
	}
	closedir(d);
	if (recdepth > 0 && !secondpass) {
		secondpass = 1;
		goto StartScan;
	}
	return NULL;
}

/* find input file; leave name in pathname[] */
FILE *findfile(char *fnam, char **dirpath, char **suffixes) {
        char **dp, *dir, **sp;
	FILE *fp;
	int dl, recdepth;
#ifndef __klibc__
	struct decompressor *dc;
#endif /* __klibc__ */

	if (strlen(fnam) >= sizeof(pathname))
		return NULL;

	/* Try explicitly given name first */
	strcpy(pathname, fnam);
	fp = maybe_pipe_open();
	if (fp)
		return fp;

	/* Test for full pathname - opening it failed, so need suffix */
	/* (This is just nonsense, for backwards compatibility.) */
	if (*fnam == '/') {
	    struct stat statbuf;

	    for (sp = suffixes; *sp; sp++) {
		if (strlen(fnam) + strlen(*sp) + 1 > sizeof(pathname))
		    continue;
		if (*sp == 0)
		    continue;	/* we tried it already */
		sprintf(pathname, "%s%s", fnam, *sp);
		if(stat(pathname, &statbuf) == 0 && S_ISREG(statbuf.st_mode)
		   && (fp = fopen(pathname, "r")) != NULL)
		    return fp;
	    }

#ifndef __klibc__
	    for (sp = suffixes; *sp; sp++) {
		for (dc = &decompressors[0]; dc->cmd; dc++) {
		    if (strlen(fnam) + strlen(*sp)
			+ strlen(dc->ext) + 1 > sizeof(pathname))
			    continue;
		    sprintf(pathname, "%s%s%s", fnam, *sp, dc->ext);
		    if (stat(pathname, &statbuf) == 0
			&& S_ISREG(statbuf.st_mode)
			&& (fp = fopen(pathname, "r")) != NULL) {
			    fclose(fp);
			    return pipe_open(dc);
		    }
		}
	    }
#endif /* __klibc__ */

	    return NULL;
	}

	/* Search a list of directories and directory hierarchies */
	for (dp = dirpath; *dp; dp++) {

	    /* delete trailing slashes; trailing stars denote recursion */
	    dir = xstrdup(*dp);
	    dl = strlen(dir);
	    recdepth = 0;
	    while (dl && dir[dl-1] == '*') {
		    dir[--dl] = 0;
		    recdepth++;
	    }
	    if (dl == 0) {
		    dir = ".";
	    } else if (dl > 1 && dir[dl-1] == '/') {
		    dir[dl-1] = 0;
	    }

	    fp = findfile_in_dir(fnam, dir, recdepth, suffixes);
	    if (fp)
		    return fp;
	}

	return NULL;
}
