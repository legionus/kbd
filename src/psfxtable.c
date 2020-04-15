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

int main(int argc, char **argv)
{
	const char *ifname, *ofname, *itname, *otname;
	FILE *ifil, *ofil, *itab, *otab;
	int psftype, hastable, notable;
	int i, ret;
	unsigned int width = 8, bytewidth, height, charsize, fontlen;
	unsigned char *inbuf, *fontbuf;
	unsigned int inbuflth, fontbuflth;
	struct unicode_list *uclistheads = NULL;

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

	if (kfont_readpsffont(&ctx, ifil, &inbuf, &inbuflth, &fontbuf,
				&fontbuflth, &width, &fontlen, 0,
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
		ret = kfont_read_itable(&ctx, itab, fontlen, &uclistheads);
		if (ret < 0)
			exit(-ret);
		fclose(itab);
	}

	if (otab) {
		if (!hastable) {
			KFONT_ERR(&ctx, _("input font does not have an index"));
			exit(EX_DATAERR);
		}

		kfont_write_itable(&ctx, otab, fontlen, uclistheads);
		fclose(otab);
	}

	if (ofil) {
		ret = kfont_writepsffont(&ctx, ofil, fontbuf, width, height,
				fontlen, psftype, notable ? NULL : uclistheads);
		fclose(ofil);
		if (ret < 0)
			return -ret;
	}

	return EX_OK;
}
