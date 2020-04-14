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
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sysexits.h>

#include "libcommon.h"

#include "kfont.h"
#include "unicode.h"
#include "psf.h"
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

static long
getunicode(char **p0)
{
	char *p = *p0;

	while (*p == ' ' || *p == '\t')
		p++;
	if (*p != 'U' || p[1] != '+' ||
	    !isxdigit(p[2]) || !isxdigit(p[3]) || !isxdigit(p[4]) ||
	    !isxdigit(p[5]) || isxdigit(p[6]))
		return -1;
	*p0 = p + 6;
	return strtol(p + 2, 0, 16);
}

static void
parse_itab_line(struct kfont_context *ctx, char *buf, unsigned int fontlen)
{
	char *p, *p1;
	long i;
	long fp0, fp1, un0, un1;

	if ((p = strchr(buf, '\n')) != NULL)
		*p = 0;
	else {
		KFONT_ERR(ctx, _("Warning: line too long"));
		exit(EX_DATAERR);
	}

	p = buf;

	while (*p == ' ' || *p == '\t')
		p++;
	if (!*p || *p == '#')
		return;

	fp0 = strtol(p, &p1, 0);
	if (p1 == p) {
		KFONT_ERR(ctx, _("Bad input line: %s"), buf);
		exit(EX_DATAERR);
	}
	p = p1;

	if (*p == '-') {
		p++;
		fp1 = strtol(p, &p1, 0);
		if (p1 == p) {
			KFONT_ERR(ctx, _("Bad input line: %s"), buf);
			exit(EX_DATAERR);
		}
		p = p1;
	} else
		fp1 = 0;

	if (fp0 < 0 || fp0 >= fontlen) {
		KFONT_ERR(ctx, _("Glyph number (0x%lx) past end of font"), fp0);
		exit(EX_DATAERR);
	}
	if (fp1 && (fp1 < fp0 || fp1 >= fontlen)) {
		KFONT_ERR(ctx, _("Bad end of range (0x%lx)"), fp1);
		exit(EX_DATAERR);
	}

	int ret = 0;

	if (fp1) {
		/* we have a range; expect the word "idem"
		   or a Unicode range of the same length */
		while (*p == ' ' || *p == '\t')
			p++;
		if (!strncmp(p, "idem", 4)) {
			for (i = fp0; i <= fp1; i++) {
				ret = addpair(uclistheads + i, i);
				if (ret < 0) {
					KFONT_ERR(ctx, "unable to add pair: %s", strerror(-ret));
					exit(EX_OSERR);
				}
			}
			p += 4;
		} else {
			un0 = getunicode(&p);
			while (*p == ' ' || *p == '\t')
				p++;
			if (*p != '-') {
				KFONT_ERR(ctx,
				        _("Corresponding to a range of "
				          "font positions, there should be "
				          "a Unicode range"));
				exit(EX_DATAERR);
			}
			p++;
			un1 = getunicode(&p);
			if (un0 < 0 || un1 < 0) {
				KFONT_ERR(ctx,
				        _("Bad Unicode range "
				          "corresponding to font position "
				          "range 0x%lx-0x%lx"),
					fp0, fp1);
				exit(EX_DATAERR);
			}
			if (un1 - un0 != fp1 - fp0) {
				KFONT_ERR(ctx,
				        _("Unicode range U+%lx-U+%lx not "
				          "of the same length as font "
				          "position range 0x%lx-0x%lx"),
				        un0, un1, fp0, fp1);
				exit(EX_DATAERR);
			}
			for (i = fp0; i <= fp1; i++) {
				ret = addpair(uclistheads + i, un0 - fp0 + i);
				if (ret < 0) {
					KFONT_ERR(ctx, "unable to add pair: %s", strerror(-ret));
					exit(EX_OSERR);
				}
			}
		} /* not idem */
	} else {  /* no range */
		while ((un0 = getunicode(&p)) >= 0) {
			ret = addpair(uclistheads + fp0, un0);
			if (ret < 0) {
				KFONT_ERR(ctx, "unable to add pair: %s", strerror(-ret));
				exit(EX_OSERR);
			}
			while (*p++ == ',' && (un1 = getunicode(&p)) >= 0) {
				ret = addseq(uclistheads + fp0, un1);
				if (ret < 0) {
					KFONT_ERR(ctx, "unable to add sequence: %s", strerror(-ret));
					exit(EX_OSERR);
				}
			}
			p--;
		}
		while (*p == ' ' || *p == '\t')
			p++;
		if (*p && *p != '#') {
			KFONT_ERR(ctx, _("trailing junk (%s) ignored"), p);
		}
	}
}

static void
read_itable(struct kfont_context *ctx, FILE *itab, unsigned int fontlen,
		struct unicode_list **uclistheadsp)
{
	char buf[65536];
	unsigned int i;

	if (uclistheadsp) {
		*uclistheadsp = realloc(*uclistheadsp,
		                         fontlen * sizeof(struct unicode_list));
		if (!*uclistheadsp) {
			KFONT_ERR(ctx, "realloc: %m");
			exit(EX_OSERR);
		}

		for (i = 0; i < fontlen; i++) {
			struct unicode_list *up = &((*uclistheadsp)[i]);
			up->next                = NULL;
			up->seq                 = NULL;
			up->prev                = up;
		}

		while (fgets(buf, sizeof(buf), itab) != NULL)
			parse_itab_line(ctx, buf, fontlen);
	}
}

int debug = 0;

int main(int argc, char **argv)
{
	const char *ifname, *ofname, *itname, *otname;
	FILE *ifil, *ofil, *itab, *otab;
	int psftype, hastable, notable;
	int i;
	unsigned int width = 8, bytewidth, height, charsize, fontlen;
	unsigned char *inbuf, *fontbuf;
	unsigned int inbuflth, fontbuflth;

	set_progname(argv[0]);
	setuplocale();

	if (argc == 2 && !strcmp(argv[1], "-V"))
		print_version_and_exit();

	ifil = ofil = itab = otab = NULL;
	ifname = ofname = itname = otname = NULL;
	fontbuf                           = NULL;
	notable                           = 0;

	struct kfont_context ctx;
	kfont_init(&ctx);

	if (!strcmp(get_progname(), "psfaddtable")) {
		/* Do not send binary data to stdout without explicit "-" */
		if (argc != 4) {
			const char *u = _("Usage:\n\t%s infont intable outfont\n");
			fprintf(stderr, u, get_progname());
			exit(EX_USAGE);
		}
		ifname = argv[1];
		itname = argv[2];
		ofname = argv[3];
	} else if (!strcmp(get_progname(), "psfgettable")) {
		if (argc < 2 || argc > 3) {
			const char *u = _("Usage:\n\t%s infont [outtable]\n");
			fprintf(stderr, u, get_progname());
			exit(EX_USAGE);
		}
		ifname = argv[1];
		otname = (argc == 3) ? argv[2] : "-";
	} else if (!strcmp(get_progname(), "psfstriptable")) {
		/* Do not send binary data to stdout without explicit "-" */
		if (argc != 3) {
			const char *u = _("Usage:\n\t%s infont outfont\n");
			fprintf(stderr, u, get_progname());
			exit(EX_USAGE);
		}
		ifname  = argv[1];
		ofname  = argv[2];
		notable = 1;
	} else {
		for (i = 1; i < argc; i++) {
			if ((!strcmp(argv[i], "-i") || !strcmp(argv[i], "-if")) && i < argc - 1)
				ifname = argv[++i];
			else if ((!strcmp(argv[i], "-o") || !strcmp(argv[i], "-of")) && i < argc - 1)
				ofname = argv[++i];
			else if (!strcmp(argv[i], "-it") && i < argc - 1)
				itname = argv[++i];
			else if (!strcmp(argv[i], "-ot") && i < argc - 1)
				otname = argv[++i];
			else if (!strcmp(argv[i], "-nt"))
				notable = 1;
			else
				break;
		}
		if (i < argc || argc <= 1) {
			const char *u = _("Usage:\n\t%s [-i infont] [-o outfont] "
			                  "[-it intable] [-ot outtable] [-nt]\n");
			fprintf(stderr, u, get_progname());
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
			KFONT_ERR(&ctx, "Unable to open: %s: %m", ifname);
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
			KFONT_ERR(&ctx, "Unable to open: %s: %m", itname);
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
			KFONT_ERR(&ctx, "Unable to open: %s: %m", ofname);
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
			KFONT_ERR(&ctx, "Unable to open: %s: %m", otname);
			exit(EX_CANTCREAT);
		}
	}

	if (readpsffont(&ctx, ifil, &inbuf, &inbuflth, &fontbuf, &fontbuflth,
	                &width, &fontlen, 0,
	                itab ? NULL : &uclistheads) < 0) {
		KFONT_ERR(&ctx, _("Bad magic number on %s"), ifname);
		exit(EX_DATAERR);
	}
	fclose(ifil);

	charsize  = fontbuflth / fontlen;
	bytewidth = (width + 7) / 8;
	if (!bytewidth)
		bytewidth = 1;
	height            = charsize / bytewidth;

	hastable = (uclistheads != NULL);

	if (PSF1_MAGIC_OK((unsigned char *)inbuf)) {
		psftype = 1;
	} else if (PSF2_MAGIC_OK((unsigned char *)inbuf)) {
		psftype = 2;
	} else {
		KFONT_ERR(&ctx, _("psf file with unknown magic"));
		exit(EX_DATAERR);
	}

	if (itab) {
		read_itable(&ctx, itab, fontlen, &uclistheads);
		fclose(itab);
	}

	if (otab) {
		struct unicode_list *ul;
		struct unicode_seq *us;
		const char *sep;

		if (!hastable) {
			KFONT_ERR(&ctx, _("input font does not have an index"));
			exit(EX_DATAERR);
		}
		fprintf(otab,
		        "#\n# Character table extracted from font %s\n#\n",
		        ifname);
		for (i = 0; i < fontlen; i++) {
			fprintf(otab, "0x%03x\t", i);
			sep = "";
			ul  = uclistheads[i].next;
			while (ul) {
				us = ul->seq;
				while (us) {
					fprintf(otab, "%sU+%04x", sep, us->uc);
					us  = us->next;
					sep = ", ";
				}
				ul  = ul->next;
				sep = " ";
			}
			fprintf(otab, "\n");
		}
		fclose(otab);
	}

	if (ofil) {
		int ret;
		ret = writepsffont(&ctx, ofil, fontbuf, width, height, fontlen,
				psftype, notable ? NULL : uclistheads);
		fclose(ofil);
		if (ret < 0)
			return -ret;
	}

	return EX_OK;
}
