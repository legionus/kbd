/*
 * loadunimap.c - aeb
 *
 * Version 1.09
 */
#include "config.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/kd.h>

#include <kbdfile.h>

#include "paths.h"
#include "psffontop.h"
#include "utf8.h"
#include "psf.h"

#include "libcommon.h"
#include "kfont.h"

extern char *progname;
extern int force;

static const char *const unidirpath[]  = {
	"",
	DATADIR "/" UNIMAPDIR "/",
	NULL
};
static const char *const unisuffixes[] = {
	"",
	".uni",
	".sfm",
	NULL
};

#ifdef MAIN
int verbose = 0;
int force   = 0;
int debug   = 0;

static void __attribute__((noreturn))
usage(void)
{
	fprintf(stderr,
	        _("Usage:\n\t%s [-C console] [-o map.orig]\n"), progname);
	exit(1);
}

int main(int argc, char *argv[])
{
	int fd, c;
	char *console = NULL;
	char *outfnam = NULL;
	const char *infnam  = "def.uni";

	set_progname(argv[0]);
	setuplocale();

	if (argc == 2 &&
	    (!strcmp(argv[1], "-V") || !strcmp(argv[1], "--version")))
		print_version_and_exit();

	while ((c = getopt(argc, argv, "C:o:")) != EOF) {
		switch (c) {
			case 'C':
				console = optarg;
				break;
			case 'o':
				outfnam = optarg;
				break;
			default:
				usage();
		}
	}

	if (argc > optind + 1 || (argc == optind && !outfnam))
		usage();

	if ((fd = getfd(console)) < 0)
		kbd_error(EXIT_FAILURE, 0, _("Couldn't get a file descriptor referring to the console"));

	struct kfont_context ctx = {
		.progname = get_progname(),
		.log_fn = log_stderr,
	};

	if (outfnam) {
		saveunicodemap(&ctx, fd, outfnam);
		if (argc == optind)
			exit(0);
	}

	if (argc == optind + 1)
		infnam = argv[optind];
	loadunicodemap(&ctx, fd, infnam);
	exit(0);
}
#endif

/*
 * Skip spaces and read U+1234 or return -1 for error.
 * Return first non-read position in *p0 (unchanged on error).
 */
static int
getunicode(char **p0)
{
	char *p = *p0;

	while (*p == ' ' || *p == '\t')
		p++;
#if 0
	/* The code below also allows one to accept 'utf8' */
	if (*p == '\'') {
		int err;
		unsigned long u;
		char *p1 = p+1;

		/*
		 * Read a single complete utf-8 character, and
		 * expect it to be closed by a single quote.
		 */
		u = from_utf8(&p1, 0, &err);
		if (err || *p1 != '\'')
			return -1;
		*p0 = p1+1;
		return u;
	}
#endif
	if (*p != 'U' || p[1] != '+' || !isxdigit(p[2]) || !isxdigit(p[3]) ||
	    !isxdigit(p[4]) || !isxdigit(p[5]) || isxdigit(p[6]))
		return -1;
	*p0 = p + 6;
	return strtol(p + 2, 0, 16);
}

static struct unimapdesc descr;

static struct unipair *list = 0;
static unsigned int listsz  = 0;
static unsigned int listct  = 0;

static void
add_unipair(struct kfont_context *ctx, int fp, int un)
{
	if (listct == listsz) {
		listsz += 4096;
		list = realloc(list, listsz);
		if (!list) {
			ERR(ctx, "realloc: %m");
			exit(EX_OSERR);
		}
	}
	list[listct].fontpos = fp;
	list[listct].unicode = un;
	listct++;
}

/*
 * Syntax accepted:
 *	<fontpos>	<unicode> <unicode> ...
 *	<range>		idem
 *	<range>		<unicode>
 *	<range>		<unicode range>
 *
 * where <range> ::= <fontpos>-<fontpos>
 * and <unicode> ::= U+<h><h><h><h>
 * and <h> ::= <hexadecimal digit>
 */

static void
parseline(struct kfont_context *ctx, char *buffer, const char *tblname)
{
	int fontlen = 512;
	int i;
	int fp0, fp1, un0, un1;
	char *p, *p1;

	p = buffer;

	while (*p == ' ' || *p == '\t')
		p++;
	if (!*p || *p == '#')
		return; /* skip comment or blank line */

	fp0 = strtol(p, &p1, 0);
	if (p1 == p) {
		ERR(ctx, _("Bad input line: %s"), buffer);
		exit(EX_DATAERR);
	}
	p = p1;

	while (*p == ' ' || *p == '\t')
		p++;
	if (*p == '-') {
		p++;
		fp1 = strtol(p, &p1, 0);
		if (p1 == p) {
			ERR(ctx, _("Bad input line: %s"), buffer);
			exit(EX_DATAERR);
		}
		p = p1;
	} else
		fp1 = 0;

	if (fp0 < 0 || fp0 >= fontlen) {
		ERR(ctx, _("%s: Glyph number (0x%x) larger than font length"),
		    tblname, fp0);
		exit(EX_DATAERR);
	}
	if (fp1 && (fp1 < fp0 || fp1 >= fontlen)) {
		ERR(ctx, _("%s: Bad end of range (0x%x)\n"),
		    tblname, fp1);
		exit(EX_DATAERR);
	}

	if (fp1) {
		/* we have a range; expect the word "idem" or a Unicode range
		   of the same length or a single Unicode value */
		while (*p == ' ' || *p == '\t')
			p++;
		if (!strncmp(p, "idem", 4)) {
			p += 4;
			for (i = fp0; i <= fp1; i++)
				add_unipair(ctx, i, i);
			goto lookattail;
		}

		un0 = getunicode(&p);
		while (*p == ' ' || *p == '\t')
			p++;
		if (*p != '-') {
			for (i = fp0; i <= fp1; i++)
				add_unipair(ctx, i, un0);
			goto lookattail;
		}

		p++;
		un1 = getunicode(&p);
		if (un0 < 0 || un1 < 0) {
			ERR(ctx,
			        _("%s: Bad Unicode range corresponding to "
			          "font position range 0x%x-0x%x"),
			        tblname, fp0, fp1);
			exit(EX_DATAERR);
		}
		if (un1 - un0 != fp1 - fp0) {
			ERR(ctx,
			        _("%s: Unicode range U+%x-U+%x not of the same"
			          " length as font position range 0x%x-0x%x"),
			        tblname, un0, un1, fp0, fp1);
			exit(EX_DATAERR);
		}
		for (i = fp0; i <= fp1; i++)
			add_unipair(ctx, i, un0 - fp0 + i);

	} else {
		/* no range; expect a list of unicode values
		   for a single font position */

		while ((un0 = getunicode(&p)) >= 0)
			add_unipair(ctx, fp0, un0);
	}
lookattail:
	while (*p == ' ' || *p == '\t')
		p++;
	if (*p && *p != '#')
		ERR(ctx, _("%s: trailing junk (%s) ignored"), tblname, p);
}

void loadunicodemap(struct kfont_context *ctx, int fd, const char *tblname)
{
	char buffer[65536];
	char *p;
	struct kbdfile *fp;

	if ((fp = kbdfile_new(NULL)) == NULL)
		nomem();

	if (kbdfile_find(tblname, unidirpath, unisuffixes, fp)) {
		ERR(ctx, "unable to find unimap: %s: %m", tblname);
		exit(EX_NOINPUT);
	}

	if (verbose)
		INFO(ctx, _("Loading unicode map from file %s"), kbdfile_get_pathname(fp));

	while (fgets(buffer, sizeof(buffer), kbdfile_get_file(fp)) != NULL) {
		if ((p = strchr(buffer, '\n')) != NULL)
			*p = '\0';
		else
			WARN(ctx, _("%s: Warning: line too long"), tblname);

		parseline(ctx, buffer, tblname);
	}

	kbdfile_free(fp);

	if (listct == 0 && !force) {
		ERR(ctx,
		        _("not loading empty unimap\n"
		          "(if you insist: use option -f to override)"));
	} else {
		descr.entry_ct = listct;
		descr.entries  = list;
		if (loadunimap(ctx, fd, NULL, &descr))
			exit(1);
		listct = 0;
	}
}

static int
getunicodemap(struct kfont_context *ctx, int fd, struct unimapdesc *unimap_descr)
{
	if (getunimap(ctx, fd, unimap_descr))
		return -1;

#ifdef MAIN
	fprintf(stderr, "# %d %s\n", unimap_descr->entry_ct,
	        (unimap_descr->entry_ct == 1) ? _("entry") : _("entries"));
#endif
	return 0;
}

void saveunicodemap(struct kfont_context *ctx, int fd, char *oufil)
{
	FILE *fpo;
	struct unimapdesc unimap_descr = { 0 };
	struct unipair *unilist;
	int i;

	if ((fpo = fopen(oufil, "w")) == NULL) {
		ERR(ctx, "unable to open file: %s", oufil);
		exit(1);
	}

	if (getunicodemap(ctx, fd, &unimap_descr) < 0)
		exit(1);

	unilist = unimap_descr.entries;

	for (i = 0; i < unimap_descr.entry_ct; i++)
		fprintf(fpo, "0x%02x\tU+%04x\n", unilist[i].fontpos, unilist[i].unicode);
	fclose(fpo);

	if (verbose)
		printf(_("Saved unicode map on `%s'\n"), oufil);
}

void appendunicodemap(struct kfont_context *ctx, int fd, FILE *fp,
                      unsigned int fontsize, int utf8)
{
	struct unimapdesc unimap_descr = { 0 };
	struct unipair *unilist;
	unsigned int i;
	int j, ret;

	if (getunicodemap(ctx, fd, &unimap_descr) < 0)
		exit(1);

	unilist = unimap_descr.entries;

	for (i = 0; i < fontsize; i++) {
#if 0
		/* More than one mapping is not a sequence! */
		int no = 0;
		for(j=0; j<unimap_descr.entry_ct; j++)
			if (unilist[j].fontpos == i)
				no++;
		if (no > 1)
			appendseparator(fp, 1, utf8);
#endif
		if (debug)
			printf("\nchar %03x: ", i);
		for (j = 0; j < unimap_descr.entry_ct; j++)
			if (unilist[j].fontpos == i) {
				if (debug)
					printf("%04x ", unilist[j].unicode);
				ret = appendunicode(ctx,fp, unilist[j].unicode, utf8);
				if (ret < 0)
					exit(-ret);
			}
		ret = appendseparator(ctx, fp, 0, utf8);
		if (ret < 0)
			exit(-ret);
	}

	if (debug)
		printf("\n");
	if (verbose)
		printf(_("Appended Unicode map\n"));
}
