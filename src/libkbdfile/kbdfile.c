#include "config.h"

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

#include <kbdfile.h>

#include "contextP.h"

static struct decompressor {
	unsigned char magic[2];
	FILE *(*decompressor)(struct kbdfile *fp);
	const char *ext; /* starts with `.', has no other dots */
	const char *cmd;
} decompressors[] = {
	{ { 0x1f, 0x8b }, kbdfile_decompressor_zlib,  ".gz",  "gzip -d -c"    },
	{ { 0x1f, 0x9e }, kbdfile_decompressor_zlib,  ".gz",  "gzip -d -c"    },
	{ { 0x42, 0x5a }, kbdfile_decompressor_bzip2, ".bz2", "bzip2 -d -c"   },
	{ { 0xfd, 0x37 }, kbdfile_decompressor_lzma,  ".xz",  "xz -d -c"      },
	{ { 0x28, 0xb5 }, kbdfile_decompressor_zstd,  ".zst", "zstd -d -q -c" },
	{ { 0, 0 }, NULL, NULL, NULL }
};

struct kbdfile *
kbdfile_new(struct kbdfile_ctx *ctx)
{
	struct kbdfile *fp = calloc(1, sizeof(struct kbdfile));

	if (!fp)
		return NULL;

	fp->ctx = ctx;

	if (!fp->ctx) {
		fp->ctx = kbdfile_context_new();
		if (!fp->ctx) {
			free(fp);
			return NULL;
		}
		fp->flags |= KBDFILE_CTX_INITIALIZED;
	}

	return fp;
}

void
kbdfile_free(struct kbdfile *fp)
{
	if (!fp)
		return;
	if (fp->flags & KBDFILE_CTX_INITIALIZED)
		kbdfile_context_free(fp->ctx);
	kbdfile_close(fp);
	free(fp);
}

char *
kbdfile_get_pathname(struct kbdfile *fp)
{
	if (!fp)
		return NULL;
	return fp->pathname;
}

int
kbdfile_set_pathname(struct kbdfile *fp, const char *pathname)
{
	strncpy(fp->pathname, pathname, sizeof(fp->pathname) - 1);
	return 0;
}

static int KBD_ATTR_PRINTF(2, 3)
kbdfile_pathname_sprintf(struct kbdfile *fp, const char *fmt, ...)
{
	ssize_t size;
	va_list ap;

	if (fp == NULL || fmt == NULL)
		return -1;

	va_start(ap, fmt);
	size = vsnprintf(NULL, 0, fmt, ap);
	va_end(ap);

	if (size < 0 || (size_t) size >= sizeof(fp->pathname))
		return -1;

	va_start(ap, fmt);
	vsnprintf(fp->pathname, sizeof(fp->pathname), fmt, ap);
	va_end(ap);

	return 0;
}

FILE *
kbdfile_get_file(struct kbdfile *fp)
{
	if (!fp)
		return NULL;
	return fp->fd;
}

int
kbdfile_set_file(struct kbdfile *fp, FILE *x)
{
	fp->fd = x;
	return 0;
}

void
kbdfile_close(struct kbdfile *fp)
{
	if (!fp || !fp->fd)
		return;
	if (fp->flags & KBDFILE_PIPE)
		pclose(fp->fd);
	else
		fclose(fp->fd);
	fp->fd = NULL;
	fp->pathname[0] = '\0';
}

char *
kbd_strerror(int errnum, char *buf, size_t buflen)
{
	*buf = '\0';

#if defined(__GLIBC__) && defined(_GNU_SOURCE)
	return strerror_r(errnum, buf, buflen);
#else
	int sv_errno = errno;
	int ret = strerror_r(errnum, buf, buflen);
	if (ret != 0)
		snprintf(buf, buflen, "strerror_r: errno=%d", (ret == -1) ? errno : ret);
	errno = sv_errno;
#endif
	return buf;
}

static int
pipe_open(const struct decompressor *dc, struct kbdfile *fp)
{
	char *pipe_cmd;

	pipe_cmd = malloc(strlen(dc->cmd) + strlen(fp->pathname) + 2);
	if (pipe_cmd == NULL)
		return -1;

	sprintf(pipe_cmd, "%s %s", dc->cmd, fp->pathname);

	fp->fd = popen(pipe_cmd, "r");
	fp->flags |= KBDFILE_PIPE | KBDFILE_COMPRESSED;

	if (!(fp->fd)) {
		char buf[200];
		ERR(fp->ctx, "popen: %s: %s", pipe_cmd, kbd_strerror(errno, buf, sizeof(buf)));
		free(pipe_cmd);
		return -1;
	}

	free(pipe_cmd);
	return 0;
}

static int
open_pathname(struct kbdfile *fp)
{
	FILE *f;
	char buf[200];
	unsigned char magic[2];
	struct stat st;

	if (!fp || stat(fp->pathname, &st) < 0 || !S_ISREG(st.st_mode))
		return -1;

	errno = 0;
	f = fopen(fp->pathname, "r");

	if (!f) {
		ERR(fp->ctx, "fopen: %s: %s", fp->pathname, kbd_strerror(errno, buf, sizeof(buf)));
		return -1;
	}

	fp->flags &= ~KBDFILE_PIPE;
	fp->flags &= ~KBDFILE_COMPRESSED;

	if ((size_t) st.st_size > sizeof(magic)) {
		struct decompressor *dc;

		errno = 0;

		if (fread(magic, sizeof(magic), 1, f) != 1) {
			ERR(fp->ctx, "fread: %s: %s", fp->pathname, kbd_strerror(errno, buf, sizeof(buf)));
			fclose(f);
			return -1;
		}

		/*
		 * We ignore the suffix and use archive magics to avoid problems
		 * with incorrect file naming.
		 */
		for (dc = &decompressors[0]; dc->cmd; dc++) {
			if (memcmp(magic, dc->magic, sizeof(magic)) != 0)
				continue;
			fclose(f);

			if (dc->decompressor && (f = dc->decompressor(fp)) != NULL) {
				fp->flags |= KBDFILE_COMPRESSED;
				goto uncompressed;
			}

			if (getenv("KBDFILE_IGNORE_DECOMP_UTILS") != NULL)
				return -1;

			return pipe_open(dc, fp);
		}

		rewind(f);
	}

uncompressed:
	fp->fd = f;

	return 0;
}

/*
 * If a file PATHNAME exists, then open it.
 * If is has a `compressed' extension, then open a pipe reading it.
 */
static int
maybe_pipe_open(struct kbdfile *fp)
{
	size_t len;
	struct decompressor *dc;

	if (!fp)
		return -1;

	if (!open_pathname(fp))
		return 0;

	len = strlen(fp->pathname);

	/*
	 * We no longer rely on suffixes to select a decompressor, but we still
	 * need to check the suffix for backward compatibility.
	 */
	for (dc = &decompressors[0]; dc->cmd; dc++) {
		if (len + strlen(dc->ext) >= sizeof(fp->pathname))
			continue;

		fp->pathname[len] = '\0';
		strcat(fp->pathname, dc->ext);

		if (!open_pathname(fp))
			return 0;
	}

	fp->pathname[len] = '\0';

	return -1;
}

static int
findfile_by_fullname(const char *fnam, const char *const *suffixes, struct kbdfile *fp)
{
	int i;

	fp->flags &= ~KBDFILE_PIPE;
	fp->flags &= ~KBDFILE_COMPRESSED;

	for (i = 0; suffixes[i]; i++) {
		if (suffixes[i] == NULL)
			continue; /* we tried it already */

		if (kbdfile_pathname_sprintf(fp, "%s%s", fnam, suffixes[i]) < 0)
			continue;

		if (!maybe_pipe_open(fp))
			return 0;
	}

	return -1;
}

static int
filecmp(const char *fname, const char *name, const char *const *suf, unsigned int *index)
{
	/* Does d_name start right? */
	const char *p = name;
	const char *q = fname;

	while (*p && *p == *q)
		p++, q++;

	if (*q)
		return 1;

	/* Does tail consist of a known suffix and possibly a compression suffix? */
	for (unsigned int i = 0; suf[i]; i++) {
		if (!strcmp(p, suf[i])) {
			if (i < *index) {
				*index = i;
			}
			return 0;
		}

		size_t l = strlen(suf[i]);

		if (strncmp(p, suf[i], l))
			continue;

		for (struct decompressor *dc = &decompressors[0]; dc->cmd; dc++) {
			if (!strcmp(p + l, dc->ext)) {
				if (i < *index) {
					*index = i;
				}
				return 0;
			}
		}
	}

	return 1;
}

static int
findfile_in_dir(const char *fnam, const char *dir, const int recdepth, const char *const *suf, struct kbdfile *fp)
{
	char errbuf[200];
	char *ff, *fdir, *path;
	int rc = 1, secondpass = 0;
	size_t dir_len;

	fp->fd = NULL;
	fp->flags &= ~KBDFILE_PIPE;
	fp->flags &= ~KBDFILE_COMPRESSED;

	dir_len = strlen(dir);

	ff = strchr(fnam, '/');
	fdir = NULL;

	if (ff != NULL) {
		fdir = strndup(fnam, (size_t) (ff - fnam));

		if (fdir == NULL) {
			ERR(fp->ctx, "strndup: %s", kbd_strerror(errno, errbuf, sizeof(errbuf)));
			return -1;
		}
	}

	struct dirent **namelist = NULL;

	int dirents = scandir(dir, &namelist, NULL, alphasort);

	if (dirents < 0) {
		DBG(fp->ctx, "scandir: %s: %s", dir, kbd_strerror(errno, errbuf, sizeof(errbuf)));
		rc = -1;
		goto EndScan;
	}

	unsigned int index = UINT_MAX;

	// Scan the directory twice: first for files, then
	// for subdirectories, so that we do never search
	// a subdirectory when the directory itself already
	// contains the file we are looking for.
StartScan:
	for (int n = 0; n < dirents; n++) {
		struct stat st;

		size_t d_len = strlen(namelist[n]->d_name);

		if (d_len < 3) {
			if (!strcmp(namelist[n]->d_name, ".") || !strcmp(namelist[n]->d_name, ".."))
				continue;
		}

		if (dir_len + d_len + 2 > sizeof(fp->pathname))
			continue;

		int okdir = (ff && !strcmp(namelist[n]->d_name, fdir));

		if ((secondpass && recdepth) || okdir) {
			path = malloc(dir_len + d_len + 2);

			if (path == NULL) {
				rc = -1;
				goto EndScan;
			}

			sprintf(path, "%s/%s", dir, namelist[n]->d_name);

			if (!stat(path, &st) && S_ISDIR(st.st_mode)) {
				if (okdir) {
					rc = findfile_in_dir(ff + 1, path, 0, suf, fp);
				}

				if (rc && recdepth) {
					rc = findfile_in_dir(fnam, path, recdepth - 1, suf, fp);
				}
			}
			free(path);

			if (!rc) {
				goto EndScan;
			}
		}

		if (secondpass || ff)
			continue;

		if (kbdfile_pathname_sprintf(fp, "%s/%s", dir, namelist[n]->d_name) < 0)
			continue;

		if (stat(fp->pathname, &st) || !S_ISREG(st.st_mode))
			continue;

		if (!filecmp(fnam, namelist[n]->d_name, suf, &index)) {
			/*
			 * We cannot immediately try to open the file because
			 * the suffixes are specified in order of priority. We
			 * need to find the lowest index.
			 */
			rc = 0;
		}
	}

	if (!secondpass) {
		if (index != UINT_MAX) {
			rc = rc ?: kbdfile_pathname_sprintf(fp, "%s/%s%s", dir, fnam, suf[index]);
			rc = rc ?: maybe_pipe_open(fp);
			goto EndScan;
		}

		if (recdepth > 0) {
			secondpass = 1;
			goto StartScan;
		}
	}

EndScan:
	if (namelist != NULL) {
		for (int n = 0; n < dirents; n++)
			free(namelist[n]);
		free(namelist);
	}

	if (fdir != NULL)
		free(fdir);

	return rc;
}

int
kbdfile_find(const char *fnam, const char *const *dirpath, const char *const *suffixes, struct kbdfile *fp)
{
	int rc, i;

	if (fp->fd != NULL) {
		ERR(fp->ctx, "can't open `%s', because kbdfile already opened: %s", fnam, fp->pathname);
		return -1;
	}

	fp->flags &= ~KBDFILE_PIPE;
	fp->flags &= ~KBDFILE_COMPRESSED;

	/* Try explicitly given name first */
	kbdfile_set_pathname(fp, fnam);

	if (!maybe_pipe_open(fp))
		return 0;

	/* Test for full pathname - opening it failed, so need suffix */
	/* (This is just nonsense, for backwards compatibility.) */
	if (*fnam == '/' &&
	    !findfile_by_fullname(fnam, suffixes, fp))
		return 0;

	/* Search a list of directories and directory hierarchies */
	for (i = 0; dirpath[i]; i++) {
		int recdepth = 0;
		char *dir = NULL;

		size_t dl = strlen(dirpath[i]);

		/* trailing stars denote recursion */
		while (dl && dirpath[i][dl - 1] == '*')
			dl--, recdepth++;

		/* delete trailing slashes */
		while (dl && dirpath[i][dl - 1] == '/')
			dl--;

		if (dl)
			dir = strndup(dirpath[i], dl);
		else
			dir = strdup(".");

		if (dir == NULL) {
			char buf[200];
			ERR(fp->ctx, "strdup: %s", kbd_strerror(errno, buf, sizeof(buf)));
			return -1;
		}

		rc = findfile_in_dir(fnam, dir, recdepth, suffixes, fp);
		free(dir);

		if (!rc)
			return 0;
	}

	return 1;
}

struct kbdfile *
kbdfile_open(struct kbdfile_ctx *ctx, const char *filename)
{
	struct kbdfile *fp = kbdfile_new(ctx);

	if (!fp)
		return NULL;

	kbdfile_set_pathname(fp, filename);

	if (maybe_pipe_open(fp) < 0) {
		kbdfile_free(fp);
		return NULL;
	}

	return fp;
}

int
kbdfile_is_compressed(struct kbdfile *fp)
{
	return (fp->flags & KBDFILE_COMPRESSED);
}
