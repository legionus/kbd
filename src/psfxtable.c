/*
 * psfxtable.c
 *
 * Manipulate headers and Unicode tables for psf fonts
 *
 * Copyright (C) 1999 Andries E. Brouwer
 *  derived from sources that were
 * Copyright (C) 1994 H. Peter Anvin
 *
 * This program may be freely copied under the terms of the GNU
 * General Public License (GPL), version 2, or at your option
 * any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sysexits.h>
#include "nls.h"
#include "version.h"
#include "psf.h"
#include "xmalloc.h"
#include "psffontop.h"

/*
 * call: psfxtable -i infont -o outfont -it intable -ot outtable
 *       psfaddtable infont intable [outfont]
 *	 psfgettable infont [outtable]
 *	 psfstriptable infont [outfont]
 *
 * When outfont is requested it will get psf1 header when
 * infont had psf1 header and intable does not have sequences
 * and psf2 header otherwise.
 */

/*
 * Parse humanly readable unicode table.
 * Format: lines
 *   <fontposition><tab><uc_defs>
 * or
 *   <fontrange><tab><uc_defs>
 * or
 *   <fontrange><tab><uc_range>
 *  where
 *   <uc_defs> :: <empty> | <uc_def><space><uc_defs>
 *   <uc_def> :: <uc> | <uc>,<uc_def>
 *   <uc> :: U+<h><h><h><h>
 *   <h> :: <hexadecimal digit>
 *   <range> :: <value>-<value>
 *  Blank lines and lines starting with # are ignored.
 */
struct unicode_list *uclistheads;

static void
addpair(int fontpos, unsigned int uc) {
	struct unicode_list *ul;
	struct unicode_seq *us;

	ul = xmalloc(sizeof(struct unicode_list));
	us = xmalloc(sizeof(struct unicode_seq));
	us->uc = uc;
	us->prev = us;
	us->next = NULL;
	ul->seq = us;
	ul->prev = uclistheads[fontpos].prev;
	ul->prev->next = ul;
	ul->next = NULL;
	uclistheads[fontpos].prev = ul;
}

static void
addseq(int fontpos, unsigned int uc) {
	struct unicode_list *ul;
	struct unicode_seq *us;

	ul = uclistheads[fontpos].prev;
	us = xmalloc(sizeof(struct unicode_seq));
	us->uc = uc;
	us->prev = ul->seq->prev;
	us->prev->next = us;
	us->next = NULL;
	ul->seq->prev = us;
}

static int
getunicode(char **p0) {
	char *p = *p0;

	while (*p == ' ' || *p == '\t')
		p++;
	if (*p != 'U' || p[1] != '+' ||
	    !isxdigit(p[2]) || !isxdigit(p[3]) || !isxdigit(p[4]) ||
	    !isxdigit(p[5]) || isxdigit(p[6]))
		return -1;
	*p0 = p+6;
	return strtol(p+2,0,16);
}

static void
parse_itab_line(char *buf, int fontlen){
	char *p, *p1;
	int i;
	long fp0, fp1, un0, un1;

	if ((p = strchr(buf, '\n')) != NULL)
		*p = 0;
	else {
		char *u = _("%s: Warning: line too long\n");
		fprintf(stderr, u, progname);
		exit(EX_DATAERR);
	}

	p = buf;

	while (*p == ' ' || *p == '\t')
		p++;
	if (!*p || *p == '#')
		return;

	fp0 = strtol(p, &p1, 0);
	if (p1 == p) {
		char *u = _("%s: Bad input line: %s\n");
		fprintf(stderr, u, progname, buf);
		exit(EX_DATAERR);
	}
	p = p1;

	if (*p == '-') {
		p++;
		fp1 = strtol(p, &p1, 0);
		if (p1 == p) {
			char *u = _("%s: Bad input line: %s\n");
			fprintf(stderr, u, progname, buf);
			exit(EX_DATAERR);
		}
		p = p1;
	} else
		fp1 = 0;

	if (fp0 < 0 || fp0 >= fontlen) {
		char *u = _("%s: Glyph number (0x%lx) past end of font\n");
		fprintf(stderr, u, progname, fp0);
		exit(EX_DATAERR);
	}
	if (fp1 && (fp1 < fp0 || fp1 >= fontlen)) {
		char *u = _("%s: Bad end of range (0x%lx)\n");
		fprintf(stderr, u, progname, fp1);
		exit(EX_DATAERR);
	}

	if (fp1) {
		/* we have a range; expect the word "idem"
		   or a Unicode range of the same length */
		while (*p == ' ' || *p == '\t')
			p++;
		if (!strncmp(p, "idem", 4)) {
			for (i = fp0; i <= fp1; i++)
				addpair(i,i);
			p += 4;
		} else {
			un0 = getunicode(&p);
			while (*p == ' ' || *p == '\t')
				p++;
			if (*p != '-') {
				char *u = _("%s: Corresponding to a range of "
					    "font positions, there should be "
					    "a Unicode range\n");
				fprintf(stderr, u, progname);
				exit(EX_DATAERR);
			}
			p++;
			un1 = getunicode(&p);
			if (un0 < 0 || un1 < 0) {
				char *u = _("%s: Bad Unicode range "
					    "corresponding to font position "
					    "range 0x%x-0x%x\n");
				fprintf(stderr, u, progname, fp0, fp1);
				exit(EX_DATAERR);
			}
			if (un1 - un0 != fp1 - fp0) {
				char *u = _("%s: Unicode range U+%x-U+%x not "
					    "of the same length as font "
					    "position range 0x%x-0x%x\n");
				fprintf(stderr, u, progname,
					un0, un1, fp0, fp1);
				exit(EX_DATAERR);
			}
			for (i = fp0; i <= fp1; i++)
				addpair(i, un0-fp0+i);
		} /* not idem */
	} else {  /* no range */
		while ((un0 = getunicode(&p)) >= 0) {
			addpair(fp0, un0);
			while (*p++ == ',' && (un1 = getunicode(&p)) >= 0) {
				addseq(fp0, un1);
			}
			p--;
		}
		while (*p == ' ' || *p == '\t')
			p++;
		if (*p && *p != '#') {
			char *u = _("%s: trailing junk (%s) ignored\n");
			fprintf(stderr, u, progname, p);
		}
	}
}

static void
read_itable(FILE *itab, int fontlen, struct unicode_list **uclistheadsp) {
	char buf[65536];
	int i;

	if (uclistheadsp) {
		*uclistheadsp = xrealloc(*uclistheadsp,
					 fontlen*sizeof(struct unicode_list));
		for (i=0; i<fontlen; i++) {
			struct unicode_list *up = &((*uclistheadsp)[i]);
			up->next = NULL;
			up->seq = NULL;
			up->prev = up;
		}
		while (fgets(buf, sizeof(buf), itab) != NULL)
			parse_itab_line(buf, fontlen);
	}
}

int debug = 0;

int
main(int argc, char **argv) {
	char *ifname, *ofname, *itname, *otname;
	FILE *ifil, *ofil, *itab, *otab;
	int psftype, fontlen, charsize, hastable, notable;
	int i;
	int width = 8, bytewidth, height;
	char *inbuf, *fontbuf;
	int inbuflth, fontbuflth;

	set_progname(argv[0]);

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE_NAME, LOCALEDIR);
	textdomain(PACKAGE_NAME);

	if (argc == 2 && !strcmp(argv[1], "-V"))
		print_version_and_exit();

	ifil = ofil = itab = otab = NULL;
	ifname = ofname = itname = otname = NULL;
	fontbuf = NULL;
	notable = 0;

	if (!strcmp(progname, "psfaddtable")) {
		/* Do not send binary data to stdout without explicit "-" */
		if (argc != 4) {
			char *u = _("Usage:\n\t%s infont intable outfont\n");
			fprintf(stderr, u, progname);
			exit(EX_USAGE);
		}
		ifname = argv[1];
		itname = argv[2];
		ofname = argv[3];
	} else if (!strcmp(progname, "psfgettable")) {
		if (argc < 2 || argc > 3) {
			char *u = _("Usage:\n\t%s infont [outtable]\n");
			fprintf(stderr, u, progname);
			exit(EX_USAGE);
		}
		ifname = argv[1];
		otname = (argc == 3) ? argv[2] : "-";
	} else if (!strcmp(progname, "psfstriptable")) {
		/* Do not send binary data to stdout without explicit "-" */
		if (argc != 3) {
			char *u = _("Usage:\n\t%s infont outfont\n");
			fprintf(stderr, u, progname);
			exit(EX_USAGE);
		}
		ifname = argv[1];
		ofname = argv[2];
		notable = 1;
	} else {
		for (i = 1; i < argc; i ++) {
			if ((!strcmp(argv[i], "-i") || !strcmp(argv[i], "-if"))
			    && i < argc-1)
				ifname = argv[++i];
			else if((!strcmp(argv[i],"-o")||!strcmp(argv[i],"-of"))
				&& i < argc-1)
				ofname = argv[++i];
			else if(!strcmp(argv[i], "-it") && i < argc-1)
				itname = argv[++i];
			else if(!strcmp(argv[i], "-ot") && i < argc-1)
				otname = argv[++i];
			else if(!strcmp(argv[i], "-nt"))
				notable = 1;
			else
				break;
		}
		if (i < argc || argc <= 1) {
			char *u = _("Usage:\n\t%s [-i infont] [-o outfont] "
				    "[-it intable] [-ot outtable] [-nt]\n");
			fprintf(stderr, u, progname);
			exit(EX_USAGE);
		}
	}

	if (!ifname)
		ifname = "-";
	if (!strcmp(ifname, "-"))
		ifil = stdin;
	else {
		ifil = fopen(ifname, "r");
		if (!ifil) {
			perror(ifname);
			exit(EX_NOINPUT);
		}
	}

	if (!itname)
		/* nothing */;
	else if (!strcmp(itname, "-"))
		itab = stdin;
	else {
		itab = fopen(itname, "r");
		if (!itab) {
			perror(itname);
			exit(EX_NOINPUT);
		}
	}

	/* Refuse ifil == itab == stdin ? Perhaps not. */

	if (!ofname)
		/* nothing */;
	else if (!strcmp(ofname, "-"))
		ofil = stdout;
	else {
		ofil = fopen(ofname, "w");
		if (!ofil) {
			perror(ofname);
			exit(EX_CANTCREAT);
		}
	}

	if (!otname)
		/* nothing */;
	else if (!strcmp(otname, "-"))
		otab = stdout;
	else {
		otab = fopen(otname, "w");
		if (!otab) {
			perror(otname);
			exit(EX_CANTCREAT);
		}
	}

	if (readpsffont(ifil, &inbuf, &inbuflth, &fontbuf, &fontbuflth,
			&width, &fontlen, 0,
			itab ? NULL : &uclistheads) == -1) {
		char *u = _("%s: Bad magic number on %s\n");
		fprintf(stderr, u, progname, ifname);
		exit(EX_DATAERR);
	}
	fclose(ifil);

	charsize = fontbuflth/fontlen;
	bytewidth = (width + 7)/8;
	if (!bytewidth)
		bytewidth = 1;
	height = charsize / bytewidth;

	hastable = (uclistheads != NULL);

	if (PSF1_MAGIC_OK((unsigned char *)inbuf)) {
		psftype = 1;
	} else if (PSF2_MAGIC_OK((unsigned char *)inbuf)) {
		psftype = 2;
	} else {
		char *u = _("%s: psf file with unknown magic\n");
		fprintf(stderr, u, progname);
		exit(EX_DATAERR);
	}

	if (itab) {
		read_itable(itab, fontlen, &uclistheads);
		fclose(itab);
	}

	if (otab) {
		struct unicode_list *ul;
		struct unicode_seq *us;
		char *sep;

		if (!hastable) {
			char *u = _("%s: input font does not have an index\n");
			fprintf(stderr, u, progname);
			exit(EX_DATAERR);
		}
		fprintf(otab,
			"#\n# Character table extracted from font %s\n#\n",
			ifname);
		for (i=0; i<fontlen; i++) {
			fprintf(otab, "0x%03x\t", i);
			sep = "";
			ul = uclistheads[i].next;
			while (ul) {
				us = ul->seq;
				while(us) {
					fprintf(otab, "%sU+%04x", sep, us->uc);
					us = us->next;
					sep = ", ";
				}
				ul = ul->next;
				sep = " ";
			}
			fprintf(otab, "\n");
		}
		fclose(otab);
	}

	if (ofil) {
		writepsffont(ofil, fontbuf, width, height, fontlen, psftype,
			     notable ? NULL : uclistheads);
		fclose(ofil);
	}

	return EX_OK;
}
