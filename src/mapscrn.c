/*
 * mapscrn.c
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <fcntl.h>
#include <sysexits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/kd.h>

#include <kbdfile.h>

#include "libcommon.h"

#include "paths.h"
#include "kfont.h"
#include "utf8.h"

static int ctoi(const char *);

/* search for the map file in these directories (with trailing /) */
static const char *const mapdirpath[]  = {
	"",
	DATADIR "/" TRANSDIR "/",
	NULL
};
static const char *const mapsuffixes[] = {
	"",
	".trans",
	"_to_uni.trans",
	".acm",
	NULL
};

#ifdef MAIN
int main(int argc, char *argv[])
{
	int fd, ret;

	struct kfont_context ctx = {
		.progname = get_progname(),
		.verbose = 0,
		.log_fn = kfont_log_stderr,
	};

	set_progname(argv[0]);
	setuplocale();

	if (argc == 2 && !strcmp(argv[1], "-V"))
		print_version_and_exit();

	if (argc > 1 && !strcmp(argv[1], "-v")) {
		ctx.verbose++;
		argc--;
		argv++;
	}

	if ((fd = getfd(NULL)) < 0)
		kbd_error(EXIT_FAILURE, 0, _("Couldn't get a file descriptor referring to the console"));

	if (argc >= 3 && !strcmp(argv[1], "-o")) {
		ret = saveoldmap(&ctx, fd, argv[2]);
		if (ret < 0)
			exit(-ret);
		argc -= 2;
		argv += 2;
		if (argc == 1)
			exit(EXIT_SUCCESS);
	}

	if (argc != 2) {
		fprintf(stderr, _("usage: %s [-V] [-v] [-o map.orig] map-file\n"),
		        get_progname());
		exit(EXIT_FAILURE);
	}
	ret = loadnewmap(&ctx, fd, argv[1]);
	if (ret < 0)
		exit(-ret);
	exit(EXIT_SUCCESS);
}
#endif

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
parsemap(FILE *fp, unsigned char *buf, unsigned short *ubuf, int *u, unsigned int *lineno)
{
	char buffer[256];
	unsigned int ln = 0;
	int in, on, ret = 0;
	char *p, *q;

	while (fgets(buffer, sizeof(buffer) - 1, fp)) {
		ln++;

		if (!*u && strstr(buffer, "U+"))
			*u = 1;

		p = strtok(buffer, " \t\n");
		if (p && *p != '#') {
			q = strtok(NULL, " \t\n#");
			if (q) {
				in = ctoi(p);
				on = ctoi(q);
				if (in >= 0 && in < 256 &&
				    on >= 0 && on < 65536) {
					ubuf[in] = (unsigned short)on;
					if (on < 256)
						buf[in] = (unsigned char)on;
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
readnewmapfromfile(struct kfont_context *ctx, const char *mfil,
		unsigned char *buf, unsigned short *ubuf)
{
	struct stat stbuf;
	unsigned int lineno = 0;
	struct kbdfile *fp = NULL;
	int ret = 0;

	if (!(fp = kbdfile_new(NULL))) {
		KFONT_ERR(ctx, "Unable to create kbdfile instance: %m");
		return -EX_OSERR;
	}

	if (kbdfile_find(mfil, mapdirpath, mapsuffixes, fp)) {
		KFONT_ERR(ctx, _("Cannot open map file: %s"), mfil);
		ret = -EX_DATAERR;
		goto end;
	}

	if (stat(kbdfile_get_pathname(fp), &stbuf)) {
		KFONT_ERR(ctx, _("Cannot open map file: %s"), kbdfile_get_pathname(fp));
		ret = -EX_DATAERR;
		goto end;
	}

	if (stbuf.st_size == E_TABSZ) {
		if (ctx->verbose)
			KFONT_INFO(ctx,
				_("Loading binary direct-to-font screen map from file %s"),
				kbdfile_get_pathname(fp));

		if (fread(buf, E_TABSZ, 1, kbdfile_get_file(fp)) != 1) {
			KFONT_ERR(ctx, _("Error reading map from file `%s'"),
				kbdfile_get_pathname(fp));
			ret = -EX_IOERR;
			goto end;
		}
	} else if (stbuf.st_size == 2 * E_TABSZ) {
		if (ctx->verbose)
			KFONT_INFO(ctx,
				_("Loading binary unicode screen map from file %s"),
				kbdfile_get_pathname(fp));

		if (fread(ubuf, 2 * E_TABSZ, 1, kbdfile_get_file(fp)) != 1) {
			KFONT_ERR(ctx,
				_("Error reading map from file `%s'"),
				kbdfile_get_pathname(fp));
			ret = -EX_IOERR;
			goto end;
		}
		ret = 1;
	} else {
		if (ctx->verbose)
			KFONT_INFO(ctx, _("Loading symbolic screen map from file %s"),
				kbdfile_get_pathname(fp));

		if (parsemap(kbdfile_get_file(fp), buf, ubuf, &ret, &lineno)) {
			KFONT_ERR(ctx,
				_("Error parsing symbolic map from `%s', line %d"),
				kbdfile_get_pathname(fp), lineno);
			ret = -1;
			goto end;
		}
	}
end:
	if (fp)
		kbdfile_free(fp);

	return ret;
}

int
loadnewmap(struct kfont_context *ctx, int fd, const char *mfil)
{
	unsigned short ubuf[E_TABSZ];
	unsigned char buf[E_TABSZ];
	unsigned int i;
	int u = 0;

	/* default: trivial straight-to-font */
	for (i = 0; i < E_TABSZ; i++) {
		buf[i]  = i;
		ubuf[i] = (0xf000 + i);
	}

	if (mfil)
		u = readnewmapfromfile(ctx, mfil, buf, ubuf);

	switch (u) {
		case 1: /* yes */
			/* do we need to use loaduniscrnmap() ? */
			u = loaduniscrnmap(ctx, fd, ubuf);
			break;
		case 0: /* no */
			u = loadscrnmap(ctx, fd, buf);
			break;
	}

	return u;
}

/*
 * Read decimal, octal, hexadecimal, Unicode (U+xxxx) or character
 * ('x', x a single byte or a utf8 sequence).
 */
int ctoi(const char *s)
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
		const char *s1 = s + 1;

		i = from_utf8((const unsigned char **)&s1, 0, &err);
		if (err || s1[0] != '\'' || s1[1] != 0)
			return -1;
	}

	else
		return -1;

	return i;
}

int
saveoldmap(struct kfont_context *ctx, int fd, const char *omfil)
{
	FILE *fp;
	unsigned char buf[E_TABSZ];
	unsigned short ubuf[E_TABSZ];
	int i, havemap, haveumap;

	if ((fp = fopen(omfil, "w")) == NULL) {
		KFONT_ERR(ctx, "Unable to open file: %s: %m", omfil);
		return -EX_DATAERR;
	}

	havemap = haveumap = 1;

	if (getscrnmap(ctx, fd, buf))
		havemap = 0;

	if (getuniscrnmap(ctx, fd, ubuf))
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
			KFONT_ERR(ctx, _("Error writing map to file"));
			return -EX_IOERR;
		}
	} else if (haveumap) {
		if (fwrite(ubuf, sizeof(ubuf), 1, fp) != 1) {
			KFONT_ERR(ctx, _("Error writing map to file"));
			return -EX_IOERR;
		}
	} else {
		KFONT_ERR(ctx, _("Cannot read console map"));
		return -1;
	}

	fclose(fp);

	if (ctx->verbose)
		KFONT_INFO(ctx, _("Saved screen map in `%s'"), omfil);

	return 0;
}
