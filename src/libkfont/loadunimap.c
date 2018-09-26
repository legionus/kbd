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

#include "kfont.h"

#include "libcommon.h"

#include "contextP.h"
#include "paths.h"
#include "psffontop.h"
#include "utf8.h"

extern int force;

/*
 * Skip spaces and read U+1234 or return -1 for error.
 * Return first non-read position in *p0 (unchanged on error).
 */
static int
getunicode(char **p0)
{
	long val;
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
	if (*p != 'U' || p[1] != '+' || !isxdigit(p[2]) || !isxdigit(p[3]) || !isxdigit(p[4]) || !isxdigit(p[5]) || isxdigit(p[6]))
		return -1;
	*p0 = p + 6;

	errno = 0;
	val = strtol(p + 2, 0, 16);

	if (errno != 0 || val < 0 || val > INT_MAX) {
		return -1;
	}

	return (int) val;
}

static struct unimapdesc descr;

static struct unipair *list = 0;
static int listsz           = 0;
static int listct           = 0;

static int
addpair(int fp, int un)
{
	if (fp < 0 || un < 0)
		return -1;

	if (listct == listsz) {
		listsz += 4096;
		list = realloc((char *)list, (size_t) listsz);
		if (!list)
			return -1;
	}
	list[listct].fontpos = (unsigned short) fp;
	list[listct].unicode = (unsigned short) un;
	listct++;

	return 0;
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

static int
parseline(struct kfont_ctx *ctx, char *buffer, const char *tblname)
{
	int fontlen = 512;
	int i;
	int fp0, fp1, un0, un1;
	char *p, *p1;

	p = buffer;

	while (*p == ' ' || *p == '\t')
		p++;

	if (!*p || *p == '#')
		return 0; // skip comment or blank line

	fp0 = strtol(p, &p1, 0);
	if (p1 == p) {
		ERR(ctx, _("Bad input line: %s\n"), buffer);
		return -1; // EX_DATAERR
	}
	p = p1;

	while (*p == ' ' || *p == '\t')
		p++;
	if (*p == '-') {
		p++;
		fp1 = strtol(p, &p1, 0);
		if (p1 == p) {
			ERR(ctx, _("Bad input line: %s\n"), buffer);
			return -1; // EX_DATAERR
		}
		p = p1;
	} else
		fp1 = 0;

	if (fp0 < 0 || fp0 >= fontlen) {
		ERR(ctx, _("%s: Glyph number (0x%x) larger than font length\n"), tblname, fp0);
		return -1; // EX_DATAERR
	}
	if (fp1 && (fp1 < fp0 || fp1 >= fontlen)) {
		ERR(ctx, _("%s: Bad end of range (0x%x)\n"), tblname, fp1);
		return -1; // EX_DATAERR
	}

	if (fp1) {
		// we have a range; expect the word "idem" or a Unicode range
		// of the same length or a single Unicode value
		while (*p == ' ' || *p == '\t')
			p++;

		if (!strncmp(p, "idem", 4)) {
			p += 4;
			for (i = fp0; i <= fp1; i++) {
				if (addpair(i, i) < 0)
					return -1; // EX_SOFTWARE
			}
			goto lookattail;
		}

		un0 = getunicode(&p);

		while (*p == ' ' || *p == '\t')
			p++;

		if (*p != '-') {
			for (i = fp0; i <= fp1; i++) {
				if (addpair(i, un0) < 0)
					return -1; // EX_SOFTWARE
			}
			goto lookattail;
		}

		p++;

		un1 = getunicode(&p);

		if (un0 < 0 || un1 < 0) {
			ERR(ctx, _("%s: Bad Unicode range corresponding to "
			           "font position range 0x%x-0x%x\n"),
			         tblname, fp0, fp1);
			return -1; // EX_DATAERR
		}

		if (un1 - un0 != fp1 - fp0) {
			ERR(ctx, _("%s: Unicode range U+%x-U+%x not of the same"
			           " length as font position range 0x%x-0x%x\n"),
			           tblname, un0, un1, fp0, fp1);
			return -1; // EX_DATAERR
		}

		for (i = fp0; i <= fp1; i++) {
			if (addpair(i, un0 - fp0 + i) < 0)
				return -1; // EX_SOFTWARE
		}
	} else {
		// no range; expect a list of unicode values
		// for a single font position
		while ((un0 = getunicode(&p)) >= 0) {
			if (addpair(fp0, un0) < 0)
				return -1; // EX_SOFTWARE
		}
	}
lookattail:
	while (*p == ' ' || *p == '\t')
		p++;

	if (*p && *p != '#') {
		WARN(ctx, _("%s: trailing junk (%s) ignored\n"), tblname, p);
	}

	return 0;
}

int
kfont_loadunicodemap(struct kfont_ctx *ctx, int fd, const char *tblname, const char *const *unidirpath, const char *const *unisuffixes)
{
	char buffer[65536];
	char *p;
	struct kbdfile *fp;

	if ((fp = kbdfile_new(ctx->kbdfile_ctx)) == NULL)
		return -1; // EX_SOFTWARE

	if (kbdfile_find((char *) tblname, unidirpath, unisuffixes, fp)) {
		kbdfile_free(fp);
		return -1; // EX_DATAERR
	}

	if (ctx->verbose)
		INFO(ctx, _("Loading unicode map from file %s"), kbdfile_get_pathname(fp));

	while (fgets(buffer, sizeof(buffer), kbdfile_get_file(fp)) != NULL) {
		if ((p = strchr(buffer, '\n')) != NULL)
			*p = '\0';
		else
			WARN(ctx, _("%s: Warning: line too long"), tblname);

		if (parseline(ctx, buffer, tblname) < 0) {
			kbdfile_free(fp);
			return -1; // EX_DATAERR
		}
	}

	kbdfile_free(fp);

	if (listct == 0 && !force) {
		WARN(ctx, _("not loading empty unimap"));
	} else {
		descr.entry_ct = listct;
		descr.entries  = list;

		if (kfont_loadunimap(ctx, fd, NULL, &descr))
			return -1;

		listct = 0;
	}

	return 0;
}

static int
getunicodemap(struct kfont_ctx *ctx, int fd, struct unimapdesc *unimap_descr)
{
	if (kfont_getunimap(ctx, fd, unimap_descr))
		return -1;

	DBG(ctx, "# %d %s\n", unimap_descr->entry_ct, (unimap_descr->entry_ct == 1) ? _("entry") : _("entries"));

	return 0;
}

int
kfont_saveunicodemap(struct kfont_ctx *ctx, int fd, char *oufil)
{
	FILE *fpo;
	struct unimapdesc unimap_descr = { 0 };
	struct unipair *unilist;
	int i;

	if ((fpo = fopen(oufil, "w")) == NULL) {
		char errbuf[STACKBUF_LEN];
		strerror_r(errno, errbuf, sizeof(errbuf));
		ERR(ctx, _("Unable to open file: %s: %s"), oufil, errbuf);
		return -1;
	}

	if (getunicodemap(ctx, fd, &unimap_descr) < 0)
		return -1;

	unilist = unimap_descr.entries;

	for (i = 0; i < unimap_descr.entry_ct; i++)
		fprintf(fpo, "0x%02x\tU+%04x\n", unilist[i].fontpos, unilist[i].unicode);

	fclose(fpo);

	DBG(ctx, _("Saved unicode map on `%s'\n"), oufil);

	return 0;
}

int
kfont_appendunicodemap(struct kfont_ctx *ctx, int fd, FILE *fp, size_t fontsize, int utf8)
{
	struct unimapdesc unimap_descr = { 0 };
	struct unipair *unilist;
	unsigned int i;
	int j;

	if (getunicodemap(ctx, fd, &unimap_descr) < 0)
		return -1;

	unilist = unimap_descr.entries;

	for (i = 0; i < fontsize; i++) {
#if 0
		/* More than one mapping is not a sequence! */
		int no = 0;
		for(j=0; j<unimap_descr.entry_ct; j++)
			if (unilist[j].fontpos == i)
				no++;
		if (no > 1) {
			if (appendseparator(fp, 1, utf8) < 0)
				return -1;
		}
#endif
		DBG(ctx, "\nchar %03x: ", i);
		for (j = 0; j < unimap_descr.entry_ct; j++)
			if (unilist[j].fontpos == i) {
				DBG(ctx, "%04x ", unilist[j].unicode);

				if (appendunicode(ctx, fp, unilist[j].unicode, utf8) < 0)
					return -1;
			}
		if (appendseparator(ctx, fp, 0, utf8) < 0)
			return -1;
	}
	if (ctx->verbose)
		INFO(ctx, _("Appended Unicode map\n"));

	return 0;
}
