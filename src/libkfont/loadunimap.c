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

struct unicodemap {
	struct unipair *list;
	size_t listsz;
	unsigned short listct;
};

static int
addpair(long fp, long un, struct unicodemap *map)
{
	if (fp < 0 || un < 0)
		return -1;

	if (map->listct == map->listsz) {
		map->listsz += 4096;
		map->list = realloc(map->list, map->listsz);
		if (!map->list)
			return -1;
	}
	map->list[map->listct].fontpos = (unsigned short) fp;
	map->list[map->listct].unicode = (unsigned short) un;
	map->listct++;

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
parseline(struct kfont_ctx *ctx, char *buffer, const char *tblname, struct unicodemap *map)
{
	int fontlen = 512;
	long i;
	int un0, un1;
	char *p, *p1;
	long fp0, fp1;

	p = buffer;

	while (*p == ' ' || *p == '\t')
		p++;

	if (!*p || *p == '#')
		return 0; // skip comment or blank line

	fp0 = strtol(p, &p1, 0);
	if (p1 == p) {
		ERR(ctx, _("Bad input line: %s\n"), buffer);
		return -EX_DATAERR;
	}
	p = p1;

	while (*p == ' ' || *p == '\t')
		p++;
	if (*p == '-') {
		p++;
		fp1 = strtol(p, &p1, 0);
		if (p1 == p) {
			ERR(ctx, _("Bad input line: %s\n"), buffer);
			return -EX_DATAERR;
		}
		p = p1;
	} else
		fp1 = 0;

	if (fp0 < 0 || fp0 >= fontlen) {
		ERR(ctx, _("%s: Glyph number (0x%x) larger than font length\n"), tblname, fp0);
		return -EX_DATAERR;
	}
	if (fp1 && (fp1 < fp0 || fp1 >= fontlen)) {
		ERR(ctx, _("%s: Bad end of range (0x%x)\n"), tblname, fp1);
		return -EX_DATAERR;
	}

	if (fp1) {
		// we have a range; expect the word "idem" or a Unicode range
		// of the same length or a single Unicode value
		while (*p == ' ' || *p == '\t')
			p++;

		if (!strncmp(p, "idem", 4)) {
			p += 4;
			for (i = fp0; i <= fp1; i++) {
				if (addpair(i, i, map) < 0)
					return -EX_SOFTWARE;
			}
			goto lookattail;
		}

		un0 = getunicode(&p);

		while (*p == ' ' || *p == '\t')
			p++;

		if (*p != '-') {
			for (i = fp0; i <= fp1; i++) {
				if (addpair(i, un0, map) < 0)
					return -EX_SOFTWARE;
			}
			goto lookattail;
		}

		p++;

		un1 = getunicode(&p);

		if (un0 < 0 || un1 < 0) {
			ERR(ctx, _("%s: Bad Unicode range corresponding to "
			           "font position range 0x%x-0x%x\n"),
			    tblname, fp0, fp1);
			return -EX_DATAERR;
		}

		if (un1 - un0 != fp1 - fp0) {
			ERR(ctx, _("%s: Unicode range U+%x-U+%x not of the same"
			           " length as font position range 0x%x-0x%x\n"),
			    tblname, un0, un1, fp0, fp1);
			return -EX_DATAERR;
		}

		for (i = fp0; i <= fp1; i++) {
			if (addpair(i, un0 - fp0 + i, map) < 0)
				return -EX_SOFTWARE;
		}
	} else {
		// no range; expect a list of unicode values
		// for a single font position
		while ((un0 = getunicode(&p)) >= 0) {
			if (addpair(fp0, un0, map) < 0)
				return -EX_SOFTWARE;
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
kfont_load_unicodemap(struct kfont_ctx *ctx, const char *tblname)
{
	char buffer[65536];
	char *p;
	struct kbdfile *fp;
	int rc;
	static struct unimapdesc descr;
	struct unicodemap map = { 0 };

	if ((fp = kbdfile_new(ctx->kbdfile_ctx)) == NULL)
		return -EX_SOFTWARE;

	if (kbdfile_find((char *) tblname, ctx->unidirs, ctx->unisuffixes, fp)) {
		kbdfile_free(fp);
		return -EX_DATAERR;
	}

	if (ctx->verbose)
		INFO(ctx, _("Loading unicode map from file %s"), kbdfile_get_pathname(fp));

	while (fgets(buffer, sizeof(buffer), kbdfile_get_file(fp)) != NULL) {
		if ((p = strchr(buffer, '\n')) != NULL)
			*p = '\0';
		else
			WARN(ctx, _("%s: Warning: line too long"), tblname);

		rc = parseline(ctx, buffer, tblname, &map);

		if (rc < 0) {
			kbdfile_free(fp);
			return -rc;
		}
	}

	kbdfile_free(fp);

	if (map.listct == 0)
		WARN(ctx, _("loading empty unimap"));


	descr.entry_ct = map.listct;
	descr.entries = map.list;

	if (kfont_load_unimap(ctx, NULL, &descr))
		return -1;

	return 0;
}

static int
getunicodemap(struct kfont_ctx *ctx, struct unimapdesc *unimap_descr)
{
	if (kfont_get_unimap(ctx, unimap_descr))
		return -1;

	DBG(ctx, "# %d %s\n", unimap_descr->entry_ct, (unimap_descr->entry_ct == 1) ? _("entry") : _("entries"));

	return 0;
}

int
kfont_dump_unicodemap(struct kfont_ctx *ctx, char *oufil)
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

	if (getunicodemap(ctx, &unimap_descr) < 0)
		return -1;

	unilist = unimap_descr.entries;

	for (i = 0; i < unimap_descr.entry_ct; i++)
		fprintf(fpo, "0x%02x\tU+%04x\n", unilist[i].fontpos, unilist[i].unicode);

	fclose(fpo);

	DBG(ctx, _("Saved unicode map on `%s'\n"), oufil);

	return 0;
}

int
append_unicodemap(struct kfont_ctx *ctx, FILE *fp, size_t fontsize, int utf8)
{
	struct unimapdesc unimap_descr = { 0 };
	struct unipair *unilist;
	unsigned int i;
	int j;

	if (getunicodemap(ctx, &unimap_descr) < 0)
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
