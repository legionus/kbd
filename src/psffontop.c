/*
 * psffontop.c - aeb@cwi.nl, 990921
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include "xmalloc.h"
#include "nls.h"
#include "psf.h"
#include "psffontop.h"
#include "utf8.h"
#include "paths.h"

extern char *progname;

static void
addpair(struct unicode_list *up, unsigned int uc) {
	struct unicode_list *ul;
	struct unicode_seq *us;

	ul = xmalloc(sizeof(struct unicode_list));
	us = xmalloc(sizeof(struct unicode_seq));
	us->uc = uc;
	us->prev = us;
	us->next = NULL;
	ul->seq = us;
	ul->prev = up->prev;
	ul->prev->next = ul;
	ul->next = NULL;
	up->prev = ul;
}

static void
addseq(struct unicode_list *up, unsigned int uc) {
	struct unicode_seq *us;
	struct unicode_seq *usl;
	struct unicode_list *ul = up->prev;

	usl = ul->seq;
	while (usl->next) usl = usl->next;
	us = xmalloc(sizeof(struct unicode_seq));
	us->uc = uc;
	us->prev = usl;
	us->next = NULL;
	usl->next = us;
	//ul->seq->prev = us;
}

static unsigned int
assemble_int(unsigned char *ip) {
	return (ip[0] + (ip[1]<<8) + (ip[2]<<16) + (ip[3]<<24));
}

static void
store_int_le(unsigned char *ip, int num) {
	ip[0] = (num & 0xff);
	ip[1] = ((num >> 8) & 0xff);
	ip[2] = ((num >> 16) & 0xff);
	ip[3] = ((num >> 24) & 0xff);
}

static unsigned int
assemble_ucs2(char **inptr, int cnt) {
	unsigned int u1, u2;

	if (cnt < 2) {
		char *u = _("%s: short ucs2 unicode table\n");
		fprintf(stderr, u, progname);
		exit(EX_DATAERR);
	}

	u1 = (unsigned char) *(*inptr)++;
	u2 = (unsigned char) *(*inptr)++;
	return (u1 | (u2 << 8));
}

/* called with cnt > 0 and **inptr not 0xff or 0xfe */
static unsigned int
assemble_utf8(char **inptr, int cnt) {
	int err;
	unsigned long uc;
	char *u;

	uc = from_utf8(inptr, cnt, &err);
	if (err) {
		switch (err) {
		case UTF8_SHORT:
			u = _("%s: short utf8 unicode table\n");
			break;
		case UTF8_BAD:
			u = _("%s: bad utf8\n");
			break;
		default:
			u = _("%s: unknown utf8 error\n");
		}
		fprintf(stderr, u, progname);
		exit(EX_DATAERR);
	}
	return uc;
}

static void
clear_uni_entry(struct unicode_list *up) {
	up->next = NULL;
	up->seq = NULL;
	up->prev = up;
}	

/*
 * Read description of a single font position.
 */
static void
get_uni_entry(char **inptr, char **endptr, struct unicode_list *up, int utf8) {
	unsigned char uc;
	unicode unichar;
	int inseq = 0;

	up->next = NULL;
	up->seq = NULL;
	up->prev = up;

	while(1) {
		if (*endptr == *inptr) {
			char *u = _("%s: short unicode table\n");
			fprintf(stderr, u, progname);
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
			unichar = assemble_utf8(inptr, *endptr - *inptr);
		} else {
			unichar = assemble_ucs2(inptr, *endptr - *inptr);
			if (unichar == PSF1_SEPARATOR)
				break;
			if (unichar == PSF1_STARTSEQ) {
				inseq = 1;
				continue;
			}
		}
		if (inseq < 2)
			addpair(up, unichar);
		else
			addseq(up, unichar);
		if (inseq)
			inseq++;
	}
}

/*
 * Read a psf font and return >= 0 on success and -1 on failure.
 * Failure means that the font was not psf (but has been read).
 * > 0 means that the Unicode table contains sequences.
 *
 * The font is read either from file (when FONT is non-NULL)
 * or from memory (namely from *ALLBUFP of size *ALLSZP).
 * In the former case, if ALLBUFP is non-NULL, a pointer to
 * the entire fontfile contents (possibly read from pipe)
 * is returned in *ALLBUFP, and the size in ALLSZP, where this
 * buffer was allocated using malloc().
 * In FONTBUFP, FONTSZP the subinterval of ALLBUFP containing
 * the font data is given.
 * The font width is stored in FONTWIDTHP.
 * The number of glyphs is stored in FONTLENP.
 * The unicodetable is stored in UCLISTHEADSP (when non-NULL), with
 * fontpositions counted from FONTPOS0 (so that calling this several
 * times can achieve font merging).
 */
extern char *progname;

int
readpsffont(FILE *fontf, char **allbufp, int *allszp,
	    char **fontbufp, int *fontszp,
	    int *fontwidthp, int *fontlenp, int fontpos0,
	    struct unicode_list **uclistheadsp) {
	char *inputbuf = NULL;
	size_t inputbuflth = 0;
	size_t inputlth, fontlen, fontwidth, charsize, hastable, ftoffset, utf8;
	size_t i, k, n;

	/*
	 * We used to look at the length of the input file
	 * with stat(); now that we accept compressed files,
	 * just read the entire file.
	 */
	if (fontf) {
		inputbuflth = MAXFONTSIZE/4; 	/* random */
		inputbuf = xmalloc(inputbuflth);
		n = 0;

		while(1) {
			if (n >= inputbuflth) {
				inputbuflth *= 2;
				inputbuf = xrealloc(inputbuf, inputbuflth);
			}
			n += fread(inputbuf+n, 1, inputbuflth-n, fontf);
			if (ferror(fontf)) {
				char *u = _("%s: Error reading input font");
				fprintf(stderr, u, progname);
				exit(EX_DATAERR);
			}
			if (feof(fontf))
				break;
		}
		if (allbufp)
			*allbufp = inputbuf;
		if (allszp)
			*allszp = n;
		inputlth = n;
	} else {
		if (!allbufp || !allszp) {
			char *u = _("%s: Bad call of readpsffont\n");
			fprintf(stderr, u, progname);
			exit(EX_SOFTWARE);
		}
		inputbuf = *allbufp;
		inputbuflth = inputlth = n = *allszp;
	}

	if (inputlth >= sizeof(struct psf1_header) &&
	    PSF1_MAGIC_OK((unsigned char *)inputbuf)) {
		struct psf1_header *psfhdr;

		psfhdr = (struct psf1_header *) &inputbuf[0];

		if (psfhdr->mode > PSF1_MAXMODE) {
			char *u = _("%s: Unsupported psf file mode (%d)\n");
			fprintf(stderr, u, progname, psfhdr->mode);
			exit(EX_DATAERR);
		}
		fontlen = ((psfhdr->mode & PSF1_MODE512) ? 512 : 256);
		charsize = psfhdr->charsize;
		hastable = (psfhdr->mode & (PSF1_MODEHASTAB|PSF1_MODEHASSEQ));
		ftoffset = sizeof(struct psf1_header);
		fontwidth = 8;
		utf8 = 0;
	} else if (inputlth >= sizeof(struct psf2_header) &&
		   PSF2_MAGIC_OK((unsigned char *)inputbuf)) {
		struct psf2_header psfhdr;
		int flags;

		memcpy(&psfhdr, inputbuf, sizeof(struct psf2_header));

		if (psfhdr.version > PSF2_MAXVERSION) {
			char *u = _("%s: Unsupported psf version (%d)\n");
			fprintf(stderr, u, progname, psfhdr.version);
			exit(EX_DATAERR);
		}
		fontlen = assemble_int((unsigned char *) &psfhdr.length);
		charsize = assemble_int((unsigned char *) &psfhdr.charsize);
		flags = assemble_int((unsigned char *) &psfhdr.flags);
		hastable = (flags & PSF2_HAS_UNICODE_TABLE);
		ftoffset = assemble_int((unsigned char *) &psfhdr.headersize);
		fontwidth = assemble_int((unsigned char *) &psfhdr.width);
		utf8 = 1;
	} else
		return -1;	/* not psf */

	/* tests required - we divide by these */
	if (fontlen == 0) {
		char *u = _("%s: zero input font length?\n");
		fprintf(stderr, u, progname);
		exit(EX_DATAERR);
	}
	if (charsize == 0) {
		char *u = _("%s: zero input character size?\n");
		fprintf(stderr, u, progname);
		exit(EX_DATAERR);
	}
	i = ftoffset + fontlen * charsize;
	if (i > inputlth || (!hastable && i != inputlth)) {
		char *u = _("%s: Input file: bad input length (%d)\n");
		fprintf(stderr, u, progname, inputlth);
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
		return 0;	/* got font, don't need unicode_list */

	*uclistheadsp = xrealloc(*uclistheadsp,
		(fontpos0+fontlen)*sizeof(struct unicode_list));

	if (hastable) {
		char *inptr, *endptr;

		inptr = inputbuf + ftoffset + fontlen * charsize;
		endptr = inputbuf + inputlth;

		for (i=0; i<fontlen; i++) {
			k = fontpos0 + i;
			get_uni_entry(&inptr, &endptr,
				      &(*uclistheadsp)[k], utf8);
		}
		if (inptr != endptr) {
			char *u = _("%s: Input file: trailing garbage\n");
			fprintf(stderr, u, progname);
			exit(EX_DATAERR);
		}
	} else {
		for (i=0; i<fontlen; i++) {
			k = fontpos0 + i;
			clear_uni_entry(&(*uclistheadsp)[k]);
		}
	}

	return 0;		/* got psf font */
}

static int
has_sequences(struct unicode_list *uclistheads, int fontlen) {
	struct unicode_list *ul;
	struct unicode_seq *us;
	int i;

	for (i=0; i<fontlen; i++) {
		ul = uclistheads[i].next;
		while(ul) {
			us = ul->seq;
			if (us && us->next)
				return 1;
			ul = ul->next;
		}
	}
	return 0;
}

void
appendunicode(FILE *fp, unsigned int uc, int utf8) {
	int n = 6;
	unsigned char out[6];

	if (uc & ~0x7fffffff) {
		fprintf(stderr, _("appendunicode: illegal unicode %u\n"), uc);
		exit(1);
	}
	if (!utf8) {
		out[--n] = ((uc >> 8) & 0xff);
		out[--n] = (uc & 0xff);
	} else if (uc < 0x80) {
		out[--n] = uc;
	} else {
		int mask = 0x3f;
		while (uc & ~mask) {
			out[--n] = 0x80 + (uc & 0x3f);
			uc >>= 6;
			mask >>= 1;
		}
		out[--n] = ((uc + ~mask + ~mask) & 0xff);
	}
	if (fwrite(out+n, 6-n, 1, fp) != 1) {
		perror("appendunimap");
		exit(1);
	}
	if (debug) {
		printf ("(");
		if (!utf8)
			printf ("U+");
		while (n < 6) printf ("%02x ", out[n++]);
		printf (")");
	}
}

void
appendseparator(FILE *fp, int seq, int utf8) {
	int n;

	if (utf8) {
		unsigned char u = (seq ? PSF2_STARTSEQ : PSF2_SEPARATOR);
		n = fwrite(&u, sizeof(u), 1, fp);
	} else {
		unsigned short u = (seq ? PSF1_STARTSEQ : PSF1_SEPARATOR);
		n = fwrite(&u, sizeof(u), 1, fp);
	}
	if (n != 1) {
		perror("appendseparator");
		exit(1);
	}
}

void
writepsffontheader(FILE *ofil, int width, int height, int fontlen,
		   int *psftype, int flags) {
	int bytewidth, charsize, ret;

	bytewidth = (width+7)/8;
	charsize = bytewidth * height;

	if ((fontlen != 256 && fontlen != 512) || width != 8)
		*psftype = 2;

	if (*psftype == 2) {
		struct psf2_header h;
		int flags2 = 0;

		if (flags & WPSFH_HASTAB)
			flags2 |= PSF2_HAS_UNICODE_TABLE;
		h.magic[0] = PSF2_MAGIC0;
		h.magic[1] = PSF2_MAGIC1;
		h.magic[2] = PSF2_MAGIC2;
		h.magic[3] = PSF2_MAGIC3;
		store_int_le((unsigned char *) &h.version, 0);
		store_int_le((unsigned char *) &h.headersize, sizeof(h));
		store_int_le((unsigned char *) &h.flags, flags2);
		store_int_le((unsigned char *) &h.length, fontlen);
		store_int_le((unsigned char *) &h.charsize, charsize);
		store_int_le((unsigned char *) &h.width, width);
		store_int_le((unsigned char *) &h.height, height);
		ret = fwrite(&h, sizeof(h), 1, ofil);
	} else {
		struct psf1_header h;

		h.magic[0] = PSF1_MAGIC0;
		h.magic[1] = PSF1_MAGIC1;
		h.mode = 0;
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
		fprintf(stderr, _("Cannot write font file header"));
		exit(EX_IOERR);
	}
}


int
writepsffont(FILE *ofil, char *fontbuf, int width, int height, size_t fontlen,
	     int psftype, struct unicode_list *uclistheads) {
	int bytewidth, charsize, flags, utf8;
	size_t i;

	bytewidth = (width+7)/8;
	charsize = bytewidth * height;
	flags = 0;

	if (uclistheads) {
		flags |= WPSFH_HASTAB;
		if (has_sequences(uclistheads, fontlen))
			flags |= WPSFH_HASSEQ;
	}

	writepsffontheader(ofil, width, height, fontlen, &psftype, flags);
	utf8 = (psftype == 2);

	if ((fwrite(fontbuf, charsize, fontlen, ofil)) != fontlen) {
		fprintf(stderr, _("Cannot write font file"));
		exit(EX_IOERR);
	}

	/* unimaps: -1 => do nothing: caller will append map */
	if (uclistheads != NULL && uclistheads != (struct unicode_list*)-1) {
		struct unicode_list *ul;
		struct unicode_seq *us;

		for (i=0; i<fontlen; i++) {
			ul = uclistheads[i].next;
			while(ul) {
				us = ul->seq;
				if (us && us->next)
					appendseparator(ofil, 1, utf8);
				while(us) {
					appendunicode(ofil, us->uc, utf8);
					us = us->next;
				}
				ul = ul->next;
			}
			appendseparator(ofil, 0, utf8);
		}
	}
	return utf8;
}

