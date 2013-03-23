/*
 * loadunimap.c - aeb
 *
 * Version 1.09
 */

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
#include "paths.h"
#include "getfd.h"
#include "xmalloc.h"
#include "findfile.h"
#include "kdmapop.h"
#include "psffontop.h"
#include "loadunimap.h"
#include "utf8.h"
#include "psf.h"
#include "nls.h"

extern char *progname;
extern int force;

static const char *const unidirpath[] = { "", DATADIR "/" UNIMAPDIR "/", 0 };
static const char *const unisuffixes[] = { "", ".uni", ".sfm", 0 };

#ifdef MAIN
#include "version.h"
int verbose = 0;
int force = 0;
int debug = 0;

static void __attribute__ ((noreturn))
usage(void) {
        fprintf(stderr,
		_("Usage:\n\t%s [-C console] [-o map.orig]\n"), progname);
        exit(1);
}

int
main(int argc, char *argv[]) {
	int fd, c;
	char *console = NULL;
	char *outfnam = NULL;
	char *infnam = "def.uni";

	set_progname(argv[0]);

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE_NAME, LOCALEDIR);
	textdomain(PACKAGE_NAME);

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

	if (argc > optind+1 || (argc == optind && !outfnam))
		usage();

	fd = getfd(console);

	if (outfnam) {
		saveunicodemap(fd, outfnam);
		if (argc == optind)
			exit(0);
	}

	if (argc == optind+1)
		infnam = argv[optind];
	loadunicodemap(fd, infnam);
	exit(0);
}
#endif

/*
 * Skip spaces and read U+1234 or return -1 for error.
 * Return first non-read position in *p0 (unchanged on error).
 */ 
static int
getunicode(char **p0) {
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
	*p0 = p+6;
	return strtol(p+2,0,16);
}

static struct unimapdesc descr;

static struct unipair *list = 0;
static int listsz = 0;
static int listct = 0;

static void
addpair(int fp, int un) {
    if (listct == listsz) {
	listsz += 4096;
	list = xrealloc((char *)list, listsz);
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
parseline(char *buffer, char *tblname) {
	int fontlen = 512;
	int i;
	int fp0, fp1, un0, un1;
	char *p, *p1;

	p = buffer;

	while (*p == ' ' || *p == '\t')
		p++;
	if (!*p || *p == '#')
		return;	/* skip comment or blank line */

	fp0 = strtol(p, &p1, 0);
	if (p1 == p) {
		fprintf(stderr, _("Bad input line: %s\n"), buffer);
		exit(EX_DATAERR);
	}
	p = p1;

	while (*p == ' ' || *p == '\t')
		p++;
	if (*p == '-') {
		p++;
		fp1 = strtol(p, &p1, 0);
		if (p1 == p) {
			fprintf(stderr, _("Bad input line: %s\n"), buffer);
			exit(EX_DATAERR);
		}
		p = p1;
	} else
		fp1 = 0;

	if ( fp0 < 0 || fp0 >= fontlen ) {
		fprintf(stderr,
			_("%s: Glyph number (0x%x) larger than font length\n"),
			tblname, fp0);
		exit(EX_DATAERR);
	}
	if ( fp1 && (fp1 < fp0 || fp1 >= fontlen) ) {
		fprintf(stderr,
			_("%s: Bad end of range (0x%x)\n"),
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
			for (i=fp0; i<=fp1; i++)
				addpair(i,i);
			goto lookattail;
		}

		un0 = getunicode(&p);
		while (*p == ' ' || *p == '\t')
			p++;
		if (*p != '-') {
			for (i=fp0; i<=fp1; i++)
				addpair(i,un0);
			goto lookattail;
		}

		p++;
		un1 = getunicode(&p);
		if (un0 < 0 || un1 < 0) {
			fprintf(stderr,
				_("%s: Bad Unicode range corresponding to "
				  "font position range 0x%x-0x%x\n"),
				tblname, fp0, fp1);
			exit(EX_DATAERR);
		}
		if (un1 - un0 != fp1 - fp0) {
			fprintf(stderr,
				_("%s: Unicode range U+%x-U+%x not of the same"
				  " length as font position range 0x%x-0x%x\n"),
				tblname, un0, un1, fp0, fp1);
			exit(EX_DATAERR);
		}
		for(i=fp0; i<=fp1; i++)
			addpair(i,un0-fp0+i);

	} else {
		/* no range; expect a list of unicode values
		   for a single font position */

		while ( (un0 = getunicode(&p)) >= 0 )
			addpair(fp0, un0);
	}
 lookattail:
	while (*p == ' ' || *p == '\t')
		p++;
	if (*p && *p != '#')
		fprintf(stderr, _("%s: trailing junk (%s) ignored\n"),
			tblname, p);
}

void
loadunicodemap(int fd, char *tblname) {
	char buffer[65536];
	char *p;
	lkfile_t fp;

	if (lk_findfile(tblname, unidirpath, unisuffixes, &fp)) {
		perror(tblname);
		exit(EX_NOINPUT);
	}

	if (verbose)
		printf(_("Loading unicode map from file %s\n"), fp.pathname);

	while ( fgets(buffer, sizeof(buffer), fp.fd) != NULL ) {
		if ( (p = strchr(buffer, '\n')) != NULL )
			*p = '\0';
		else
			fprintf(stderr, _("%s: %s: Warning: line too long\n"),
				progname, tblname);

		parseline(buffer, tblname);
	}

	lk_fpclose(&fp);

	if (listct == 0 && !force) {
		fprintf(stderr,
			_("%s: not loading empty unimap\n"
			  "(if you insist: use option -f to override)\n"),
			progname);
	} else {
		descr.entry_ct = listct;
		descr.entries = list;
		if (loadunimap (fd, NULL, &descr))
			exit(1);
		listct = 0;
	}
}

static struct unimapdesc
getunicodemap(int fd) {
  struct unimapdesc unimap_descr;

  if (getunimap(fd, &unimap_descr))
	  exit(1);

#ifdef MAIN
  fprintf(stderr, "# %d %s\n", unimap_descr.entry_ct,
	 (unimap_descr.entry_ct == 1) ? _("entry") : _("entries"));
#endif

  return unimap_descr;
}

void
saveunicodemap(int fd, char *oufil) {
  FILE *fpo;
  struct unimapdesc unimap_descr;
  struct unipair *unilist;
  int i;

  if ((fpo = fopen(oufil, "w")) == NULL) {
      perror(oufil);
      exit(1);
  }

  unimap_descr = getunicodemap(fd);
  unilist = unimap_descr.entries;

  for(i=0; i<unimap_descr.entry_ct; i++)
      fprintf(fpo, "0x%02x\tU+%04x\n", unilist[i].fontpos, unilist[i].unicode);
  fclose(fpo);

  if (verbose)
    printf(_("Saved unicode map on `%s'\n"), oufil);
}

void
appendunicodemap(int fd, FILE *fp, int fontsize, int utf8) {
	struct unimapdesc unimap_descr;
	struct unipair *unilist;
	int i, j;

	unimap_descr = getunicodemap(fd);
	unilist = unimap_descr.entries;

		
	for(i=0; i<fontsize; i++) {
#if 0
		/* More than one mapping is not a sequence! */
		int no = 0;
		for(j=0; j<unimap_descr.entry_ct; j++) 
			if (unilist[j].fontpos == i)
				no++;
		if (no > 1)
			appendseparator(fp, 1, utf8);
#endif		
		if (debug) printf ("\nchar %03x: ", i);
		for(j=0; j<unimap_descr.entry_ct; j++)
			if (unilist[j].fontpos == i) {
				if (debug)
					printf ("%04x ", unilist[j].unicode);
				appendunicode(fp, unilist[j].unicode, utf8);
			}
		appendseparator(fp, 0, utf8);
	}


	if (debug) printf ("\n");
	if (verbose)
		printf(_("Appended Unicode map\n"));
}
