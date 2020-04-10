/*
 * psffontop.c - aeb@cwi.nl, 990921
 */
#include "config.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

#include "libcommon.h"

#include "psf.h"
#include "context.h"
#include "unicode.h"
#include "psffontop.h"
#include "utf8.h"
#include "paths.h"

static uint32_t
assemble_uint32(unsigned char *ip)
{
	return (
		(uint32_t)ip[0] +
		((uint32_t)ip[1] << 8) +
		((uint32_t)ip[2] << 16) +
		((uint32_t)ip[3] << 24)
	);
}

static void
store_int_le(unsigned char *ip, unsigned int num)
{
	ip[0] = (unsigned char)(num & 0xff);
	ip[1] = (unsigned char)((num >> 8) & 0xff);
	ip[2] = (unsigned char)((num >> 16) & 0xff);
	ip[3] = (unsigned char)((num >> 24) & 0xff);
}

static unicode
assemble_ucs2(struct kfont_context *ctx, const unsigned char **inptr, int cnt)
{
	int u1, u2;

	if (cnt < 2) {
		ERR(ctx, _("short ucs2 unicode table"));
		exit(EX_DATAERR);
	}

	u1 = *(*inptr)++;
	u2 = *(*inptr)++;

	return (u1 | (u2 << 8));
}

/* called with cnt > 0 and **inptr not 0xff or 0xfe */
static unicode
assemble_utf8(struct kfont_context *ctx, const unsigned char **inptr, int cnt)
{
	int err;
	int32_t uc;

	uc = from_utf8(inptr, cnt, &err);
	if (err) {
		switch (err) {
			case UTF8_SHORT:
				ERR(ctx, _("short utf8 unicode table"));
				break;
			case UTF8_BAD:
				ERR(ctx, _("bad utf8"));
				break;
			default:
				ERR(ctx, _("unknown utf8 error"));
		}
		exit(EX_DATAERR);
	}
	return uc;
}

/*
 * Read description of a single font position.
 */
static void
get_uni_entry(struct kfont_context *ctx,
              const unsigned char **inptr, const unsigned char **endptr,
              struct unicode_list *up, int utf8)
{
	unsigned char uc;
	unicode unichar;
	int inseq = 0;

	up->next = NULL;
	up->seq  = NULL;
	up->prev = up;

	while (1) {
		if (*endptr == *inptr) {
			ERR(ctx, _("short unicode table"));
			exit(EX_DATAERR);
		}
		if (utf8) {
			uc = *(*inptr)++;
			if (uc == PSF2_SEPARATOR)
				break;
			if (uc == PSF2_STARTSEQ) {
				inseq = 1;
				continue;
			}
			--(*inptr);
			unichar = assemble_utf8(ctx, inptr, *endptr - *inptr);
		} else {
			unichar = assemble_ucs2(ctx, inptr, *endptr - *inptr);
			if (unichar == PSF1_SEPARATOR)
				break;
			if (unichar == PSF1_STARTSEQ) {
				inseq = 1;
				continue;
			}
		}

		int ret;

		if (inseq < 2)
			ret = addpair(up, unichar);
		else
			ret = addseq(up, unichar);

		if (ret < 0) {
			ERR(ctx, "unable to unichar: %s", strerror(-ret));
			exit(EX_OSERR);
		}

		if (inseq)
			inseq++;
	}
}

extern char *progname;

int readpsffont(struct kfont_context *ctx,
                FILE *fontf, unsigned char **allbufp, unsigned int *allszp,
                unsigned char **fontbufp, unsigned int *fontszp,
                unsigned int *fontwidthp, unsigned int *fontlenp, unsigned int fontpos0,
                struct unicode_list **uclistheadsp)
{
	unsigned char *inputbuf = NULL;
	unsigned int inputbuflth = 0;
	unsigned int inputlth, fontlen, fontwidth, charsize, hastable, ftoffset;
	int utf8;
	unsigned int i, k, n;

	/*
	 * We used to look at the length of the input file
	 * with stat(); now that we accept compressed files,
	 * just read the entire file.
	 */
	if (fontf) {
		inputbuflth = MAXFONTSIZE / 4; /* random */

		inputbuf = malloc(inputbuflth);
		if (!inputbuf) {
			ERR(ctx, "malloc: %m");
			exit(EX_OSERR);
		}

		n = 0;

		while (1) {
			if (n >= inputbuflth) {
				inputbuflth *= 2;

				inputbuf = realloc(inputbuf, inputbuflth);
				if (!inputbuf) {
					ERR(ctx, "realloc: %m");
					exit(EX_OSERR);
				}
			}
			n += fread(inputbuf + n, 1, inputbuflth - n, fontf);
			if (ferror(fontf)) {
				ERR(ctx, _("Error reading input font"));
				exit(EX_DATAERR);
			}
			if (feof(fontf))
				break;
		}
		if (allbufp)
			*allbufp = inputbuf;
		if (allszp)
			*allszp = n;
		inputlth        = n;
	} else {
		if (!allbufp || !allszp) {
			ERR(ctx, _("Bad call of readpsffont"));
			exit(EX_SOFTWARE);
		}
		inputbuf    = *allbufp;
		inputbuflth = inputlth = n = *allszp;
	}

	if (inputlth >= sizeof(struct psf1_header) &&
	    PSF1_MAGIC_OK((unsigned char *)inputbuf)) {
		struct psf1_header *psfhdr;

		psfhdr = (struct psf1_header *)&inputbuf[0];

		if (psfhdr->mode > PSF1_MAXMODE) {
			ERR(ctx, _("Unsupported psf file mode (%d)"), psfhdr->mode);
			exit(EX_DATAERR);
		}
		fontlen   = ((psfhdr->mode & PSF1_MODE512) ? 512 : 256);
		charsize  = psfhdr->charsize;
		hastable  = (psfhdr->mode & (PSF1_MODEHASTAB | PSF1_MODEHASSEQ));
		ftoffset  = sizeof(struct psf1_header);
		fontwidth = 8;
		utf8      = 0;
	} else if (inputlth >= sizeof(struct psf2_header) &&
	           PSF2_MAGIC_OK((unsigned char *)inputbuf)) {
		struct psf2_header psfhdr;
		unsigned int flags;

		memcpy(&psfhdr, inputbuf, sizeof(struct psf2_header));

		if (psfhdr.version > PSF2_MAXVERSION) {
			ERR(ctx, _("Unsupported psf version (%d)"), psfhdr.version);
			exit(EX_DATAERR);
		}
		fontlen   = assemble_uint32((unsigned char *)&psfhdr.length);
		charsize  = assemble_uint32((unsigned char *)&psfhdr.charsize);
		flags     = assemble_uint32((unsigned char *)&psfhdr.flags);
		hastable  = (flags & PSF2_HAS_UNICODE_TABLE);
		ftoffset  = assemble_uint32((unsigned char *)&psfhdr.headersize);
		fontwidth = assemble_uint32((unsigned char *)&psfhdr.width);
		utf8      = 1;
	} else
		return -1; /* not psf */

	/* tests required - we divide by these */
	if (fontlen == 0) {
		ERR(ctx, _("zero input font length?"));
		exit(EX_DATAERR);
	}
	if (charsize == 0) {
		ERR(ctx, _("zero input character size?"));
		exit(EX_DATAERR);
	}
	i = ftoffset + fontlen * charsize;
	if (i > inputlth || (!hastable && i != inputlth)) {
		ERR(ctx, _("Input file: bad input length (%d)"), inputlth);
		exit(EX_DATAERR);
	}

	if (fontbufp && allbufp)
		*fontbufp = *allbufp + ftoffset;
	if (fontszp)
		*fontszp = fontlen * charsize;
	if (fontlenp)
		*fontlenp = fontlen;
	if (fontwidthp)
		*fontwidthp = fontwidth;

	if (!uclistheadsp)
		return 0; /* got font, don't need unicode_list */

	void *ptr;

	ptr = realloc(*uclistheadsp,
	              (fontpos0 + fontlen) * sizeof(struct unicode_list));
	if (!ptr) {
		ERR(ctx, "realloc: %m");
		exit(EX_OSERR);
	}
	*uclistheadsp = ptr;

	if (hastable) {
		const unsigned char *inptr, *endptr;

		inptr  = inputbuf + ftoffset + fontlen * charsize;
		endptr = inputbuf + inputlth;

		for (i = 0; i < fontlen; i++) {
			k = fontpos0 + i;
			get_uni_entry(ctx, &inptr, &endptr,
			              &(*uclistheadsp)[k], utf8);
		}
		if (inptr != endptr) {
			ERR(ctx, _("Input file: trailing garbage"));
			exit(EX_DATAERR);
		}
	} else {
		for (i = 0; i < fontlen; i++) {
			k = fontpos0 + i;
			clear_uni_entry(&(*uclistheadsp)[k]);
		}
	}

	return 0; /* got psf font */
}

static int
has_sequences(struct unicode_list *uclistheads, unsigned int fontlen)
{
	struct unicode_list *ul;
	struct unicode_seq *us;
	unsigned int i;

	for (i = 0; i < fontlen; i++) {
		ul = uclistheads[i].next;
		while (ul) {
			us = ul->seq;
			if (us && us->next)
				return 1;
			ul = ul->next;
		}
	}
	return 0;
}

void appendunicode(struct kfont_context *ctx, FILE *fp, int u, int utf8)
{
	unsigned int n = 6;
	unsigned char out[6];

	if (u < 0) {
		ERR(ctx, _("appendunicode: illegal unicode %d"), u);
		exit(1);
	}

	unsigned int uc = (unsigned int)u;

	if (!utf8) {
		out[--n] = ((uc >> 8) & 0xff);
		out[--n] = (uc & 0xff);
	} else if (uc < 0x80) {
		out[--n] = (unsigned char)uc;
	} else {
		unsigned int mask = 0x3f;
		while (uc & ~mask) {
			out[--n] = (unsigned char)(0x80u + (uc & 0x3fu));
			uc >>= 6;
			mask >>= 1;
		}
		out[--n] = ((uc + ~mask + ~mask) & 0xff);
	}
	if (fwrite(out + n, 6 - n, 1, fp) != 1) {
		ERR(ctx, "appendunimap: %m");
		exit(1);
	}
	if (debug) {
		printf("(");
		if (!utf8)
			printf("U+");
		while (n < 6)
			printf("%02x ", out[n++]);
		printf(")");
	}
}

void appendseparator(struct kfont_context *ctx, FILE *fp, int seq, int utf8)
{
	size_t n;

	if (utf8) {
		unsigned char u = (seq ? PSF2_STARTSEQ : PSF2_SEPARATOR);
		n = fwrite(&u, sizeof(u), 1, fp);
	} else {
		unsigned short u = (seq ? PSF1_STARTSEQ : PSF1_SEPARATOR);
		n = fwrite(&u, sizeof(u), 1, fp);
	}
	if (n != 1) {
		ERR(ctx, "fwrite: %m");
		exit(1);
	}
}

void writepsffontheader(struct kfont_context *ctx,
                        FILE *ofil, unsigned int width, unsigned int height,
			unsigned int fontlen, int *psftype, int flags)
{
	unsigned int bytewidth, charsize;
	size_t ret;

	bytewidth = (width + 7) / 8;
	charsize  = bytewidth * height;

	if ((fontlen != 256 && fontlen != 512) || width != 8)
		*psftype = 2;

	if (*psftype == 2) {
		struct psf2_header h;
		unsigned int flags2 = 0;

		if (flags & WPSFH_HASTAB)
			flags2 |= PSF2_HAS_UNICODE_TABLE;
		h.magic[0] = PSF2_MAGIC0;
		h.magic[1] = PSF2_MAGIC1;
		h.magic[2] = PSF2_MAGIC2;
		h.magic[3] = PSF2_MAGIC3;
		store_int_le((unsigned char *)&h.version, 0);
		store_int_le((unsigned char *)&h.headersize, sizeof(h));
		store_int_le((unsigned char *)&h.flags, flags2);
		store_int_le((unsigned char *)&h.length, fontlen);
		store_int_le((unsigned char *)&h.charsize, charsize);
		store_int_le((unsigned char *)&h.width, width);
		store_int_le((unsigned char *)&h.height, height);
		ret = fwrite(&h, sizeof(h), 1, ofil);
	} else {
		struct psf1_header h;

		h.magic[0] = PSF1_MAGIC0;
		h.magic[1] = PSF1_MAGIC1;
		h.mode     = 0;
		if (fontlen == 512)
			h.mode |= PSF1_MODE512;
		if (flags & WPSFH_HASSEQ)
			h.mode |= PSF1_MODEHASSEQ;
		else if (flags & WPSFH_HASTAB)
			h.mode |= PSF1_MODEHASTAB;
		h.charsize = charsize;
		ret = fwrite(&h, sizeof(h), 1, ofil);
	}

	if (ret != 1) {
		ERR(ctx, _("Cannot write font file header"));
		exit(EX_IOERR);
	}
}

int writepsffont(struct kfont_context *ctx,
                 FILE *ofil, unsigned char *fontbuf, unsigned int width, unsigned int height, unsigned int fontlen,
                 int psftype, struct unicode_list *uclistheads)
{
	unsigned int bytewidth, charsize, i;
	int flags, utf8;

	bytewidth = (width + 7) / 8;
	charsize  = bytewidth * height;
	flags     = 0;

	if (uclistheads) {
		flags |= WPSFH_HASTAB;
		if (has_sequences(uclistheads, fontlen))
			flags |= WPSFH_HASSEQ;
	}

	writepsffontheader(ctx, ofil, width, height, fontlen, &psftype, flags);
	utf8 = (psftype == 2);

	if ((fwrite(fontbuf, charsize, fontlen, ofil)) != fontlen) {
		ERR(ctx, _("Cannot write font file"));
		exit(EX_IOERR);
	}

	/* unimaps: -1 => do nothing: caller will append map */
	if (uclistheads != NULL && uclistheads != (struct unicode_list *)-1) {
		struct unicode_list *ul;
		struct unicode_seq *us;

		for (i = 0; i < fontlen; i++) {
			ul = uclistheads[i].next;
			while (ul) {
				us = ul->seq;
				if (us && us->next)
					appendseparator(ctx, ofil, 1, utf8);
				while (us) {
					appendunicode(ctx, ofil, us->uc, utf8);
					us = us->next;
				}
				ul = ul->next;
			}
			appendseparator(ctx, ofil, 0, utf8);
		}
	}
	return utf8;
}
