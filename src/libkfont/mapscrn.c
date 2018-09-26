/*
 * mapscrn.c
 */
#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/kd.h>

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include <kbdfile.h>

#include "libcommon.h"

#include "paths.h"
#include "utf8.h"

#include "contextP.h"

static int ctoi(char *);

/*
 * Read two-column file (index, value) with index in 0-255
 * and value in 0-65535. Acceptable representations:
 * decimal (nnn), octal (0nnn), hexadecimal (0xnnn), Unicode (U+xxxx),
 * character ('x').
 * In the character encoding, x may be a single byte binary value
 * (but not space, tab, comma, #) or a single Unicode value coded
 * as a UTF-8 sequence.
 *
 * Store values in ubuf[], and small values also in buf[].
 * Set u to 1 in case a value above 255 or a U+ occurs.
 * Set lineno to line number of first error.
 */
static int
parsemap(FILE *fp, char *buf, unsigned short *ubuf, int *u, int *lineno)
{
	char buffer[256];
	int in, on, ln, ret = 0;
	char *p, *q;

	ln = 0;
	while (fgets(buffer, sizeof(buffer) - 1, fp)) {
		ln++;
		if (!*u && strstr(buffer, "U+"))
			*u = 1;
		p          = strtok(buffer, " \t\n");
		if (p && *p != '#') {
			q = strtok(NULL, " \t\n#");
			if (q) {
				in = ctoi(p);
				on = ctoi(q);
				if (in >= 0 && in < 256 &&
				    on >= 0 && on < 65536) {
					ubuf[in] = on;
					if (on < 256)
						buf[in] = on;
					else
						*u = 1;
				} else {
					if (!ret)
						*lineno = ln;
					ret = -1;
				}
			}
		}
	}
	return ret;
}

static int
readnewmapfromfile(struct kfont_ctx *ctx, char *mfil, char *buf, unsigned short *ubuf, const char *const *mapdirpath, const char *const *mapsuffixes, int *res)
{
	struct stat stbuf;
	int lineno = 0;
	struct kbdfile *fp;
	struct kbdfile_ctx *kbdfile_ctx;

	if ((kbdfile_ctx = kbdfile_context_new()) == NULL)
		return -1;

	if ((fp = kbdfile_new(kbdfile_ctx)) == NULL)
		return -1;

	if (kbdfile_find(mfil, mapdirpath, mapsuffixes, fp)) {
		ERR(ctx, _("mapscrn: cannot open map file _%s_\n"), mfil);
		return -1;
	}
	if (stat(kbdfile_get_pathname(fp), &stbuf)) {
		char errbuf[STACKBUF_LEN];
		strerror_r(errno, errbuf, sizeof(errbuf));
		ERR(ctx, _("%s: Cannot stat map file: %s"), kbdfile_get_pathname(fp), errbuf);
		return -1;
	}
	if (stbuf.st_size == E_TABSZ) {
		if (ctx->verbose) {
			INFO(ctx,
			    _("Loading binary direct-to-font screen map "
			      "from file %s"),
			    kbdfile_get_pathname(fp));
		}
		if (fread(buf, E_TABSZ, 1, kbdfile_get_file(fp)) != 1) {
			ERR(ctx, _("Error reading map from file `%s'"), kbdfile_get_pathname(fp));
			return -1;
		}
	} else if (stbuf.st_size == 2 * E_TABSZ) {
		if (ctx->verbose) {
			INFO(ctx,
			    _("Loading binary unicode screen map "
			      "from file %s"),
			    kbdfile_get_pathname(fp));
		}
		if (fread(ubuf, 2 * E_TABSZ, 1, kbdfile_get_file(fp)) != 1) {
			ERR(ctx, _("Error reading map from file `%s'"), kbdfile_get_pathname(fp));
			return -1;
		}
		*res = 1;

	} else {
		if (ctx->verbose) {
			INFO(ctx,
			    _("Loading symbolic screen map from file %s"),
			    kbdfile_get_pathname(fp));
		}
		if (parsemap(kbdfile_get_file(fp), buf, ubuf, res, &lineno)) {
			ERR(ctx,
			    _("Error parsing symbolic map "
			      "from `%s', line %d"),
			    kbdfile_get_pathname(fp), lineno);
			return -1;
		}
	}

	kbdfile_free(fp);
	kbdfile_context_free(kbdfile_ctx);

	return 0;
}

int
kfont_loadnewmap(struct kfont_ctx *ctx, int fd, char *mfil, const char *const *mapdirpath, const char *const *mapsuffixes)
{
	unsigned short ubuf[E_TABSZ];
	char buf[E_TABSZ];
	int i, u;

	/* default: trivial straight-to-font */
	for (i = 0; i < E_TABSZ; i++) {
		buf[i]  = i;
		ubuf[i] = (0xf000 + i);
	}

	u = 0;
	if (mfil && readnewmapfromfile(ctx, mfil, buf, ubuf, mapdirpath, mapsuffixes, &u) < 0)
		return -1;

	/* do we need to use loaduniscrnmap() ? */
	if (u) {
		/* yes */
		if (kfont_loaduniscrnmap(ctx, fd, ubuf))
			return -1;
	} else {
		/* no */
		if (kfont_loadscrnmap(ctx, fd, buf))
			return -1;
	}

	return 0;
}

/*
 * Read decimal, octal, hexadecimal, Unicode (U+xxxx) or character
 * ('x', x a single byte or a utf8 sequence).
 */
int ctoi(char *s)
{
	int i;

	if ((strncmp(s, "0x", 2) == 0) &&
	    (strspn(s + 2, "0123456789abcdefABCDEF") == strlen(s + 2)))
		(void)sscanf(s + 2, "%x", &i);

	else if ((*s == '0') &&
	         (strspn(s, "01234567") == strlen(s)))
		(void)sscanf(s, "%o", &i);

	else if (strspn(s, "0123456789") == strlen(s))
		(void)sscanf(s, "%d", &i);

	else if ((strncmp(s, "U+", 2) == 0) && strlen(s) == 6 &&
	         (strspn(s + 2, "0123456789abcdefABCDEF") == 4))
		(void)sscanf(s + 2, "%x", &i);

	else if ((strlen(s) == 3) && (s[0] == '\'') && (s[2] == '\''))
		i = s[1];

	else if (s[0] == '\'') {
		int err;
		char *s1 = s + 1;

		i = from_utf8(&s1, 0, &err);
		if (err || s1[0] != '\'' || s1[1] != 0)
			return -1;
	}

	else
		return -1;

	return i;
}

int
kfont_saveoldmap(struct kfont_ctx *ctx, int fd, char *omfil)
{
	FILE *fp;
	char buf[E_TABSZ];
	unsigned short ubuf[E_TABSZ];
	int i, havemap, haveumap;

	if ((fp = fopen(omfil, "w")) == NULL) {
		char errbuf[STACKBUF_LEN];
		strerror_r(errno, errbuf, sizeof(errbuf));
		ERR(ctx, _("Unable to open file: %s: %s"), omfil, errbuf);
		return -1;
	}

	havemap = haveumap = 1;

	if (kfont_getscrnmap(ctx, fd, buf))
		havemap = 0;

	if (kfont_getuniscrnmap(ctx, fd, ubuf))
		haveumap = 0;

	if (havemap && haveumap) {
		for (i = 0; i < E_TABSZ; i++) {
			if ((ubuf[i] & ~0xff) != 0xf000) {
				havemap = 0;
				break;
			}
		}
	}

	if (havemap) {
		if (fwrite(buf, sizeof(buf), 1, fp) != 1) {
			ERR(ctx, _("Error writing map to file"));
			return -1;
		}
	} else if (haveumap) {
		if (fwrite(ubuf, sizeof(ubuf), 1, fp) != 1) {
			ERR(ctx,  _("Error writing map to file"));
			return -1;
		}
	} else {
		ERR(ctx, _("Cannot read console map"));
		return -1;
	}

	fclose(fp);

	DBG(ctx, _("Saved screen map in `%s'"), omfil);

	return 0;
}
