/*
 * psffontop.c - aeb@cwi.nl, 990921
 */

#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include "xmalloc.h"
#include "nls.h"
#include "psf.h"
#include "psffontop.h"

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
	struct unicode_list *ul;
	struct unicode_seq *us;

	ul = up->prev;
	us = xmalloc(sizeof(struct unicode_seq));
	us->uc = uc;
	us->prev = ul->seq->prev;
	us->prev->next = us;
	us->next = NULL;
	ul->seq->prev = us;
}

static unsigned int
assemble_ucs2(char **inptr, int cnt) {
	unsigned char **in = (unsigned char **) inptr;
	unsigned int u1, u2;

	if (cnt < 2) {
		char *u = _("%s: short ucs2 unicode table\n");
		fprintf(stderr, u, progname);
		exit(EX_DATAERR);
	}

	u1 = *(*in)++;
	u2 = *(*in)++;
	return (u1 | (u2 << 8));
}

/* called with cnt > 0 and **inptr not 0xff or 0xfe */
static unsigned int
assemble_utf8(char **inptr, int cnt) {
	unsigned char *in;
	unsigned int uc, uc2;
	int need, bit, bad = 0;

	in = (unsigned char *)(* inptr);
	uc = *in++;
	need = 0;
	bit = 0x80;
	while(uc & bit) {
		need++;
		bit >>= 1;
	}
	uc &= (bit-1);
	if (cnt < need) {
		char *u = _("%s: short utf8 unicode table\n");
		fprintf(stderr, u, progname);
		exit(EX_DATAERR);
	}
	if (need == 1)
		bad = 1;
	else if (need) while(--need) {
		uc2 = *in++;
		if ((uc2 & 0xc0) != 0x80) {
			bad = 1;
			break;
		}
		uc = ((uc << 6) | (uc2 & 0x3f));
	}
	if (bad) {
		char *u = _("%s: bad utf8\n");
		fprintf(stderr, u, progname);
		exit(EX_DATAERR);
	}
	*inptr = in;
	return uc;
}
	

/*
 * Read description of a single font position.
 */
static void
get_uni_entry(char **inptr, char **endptr, struct unicode_list *up, int utf8) {
	unsigned char **in = (unsigned char **) inptr;
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
			uc = *(*in)++;
			if (uc == PSF2_SEPARATOR)
				break;
			if (uc == PSF2_STARTSEQ) {
				inseq = 1;
				continue;
			}
			--(*in);
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
		if (inseq == 0)
			addpair(up, unichar);
		else {
			addseq(up, unichar);
			inseq++;
		}
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
 * The number of glyphs is stored in FONTLENP.
 * The unicodetable is stored in UCLISTHEADSP (when non-NULL), with
 * fontpositions counted from FONTPOS0 (so that calling this several
 * times can achieve font merging).
 */
extern char *progname;

int
readpsffont(FILE *fontf, char **allbufp, int *allszp,
	    char **fontbufp, int *fontszp,
	    int *fontlenp, int fontpos0,
	    struct unicode_list **uclistheadsp) {
	char *inputbuf = NULL;
	int inputbuflth = 0;
	int inputlth, fontlen, charsize, hastable, ftoffset, utf8;
	int i, k, n;

	/*
	 * We used to look at the length of the input file
	 * with stat(); now that we accept compressed files,
	 * just read the entire file.
	 */
	if (fontf) {
		inputbuflth = 16384; 	/* random */
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
		utf8 = 0;
	} else if (inputlth >= sizeof(struct psf2_header) &&
		   PSF2_MAGIC_OK((unsigned char *)inputbuf)) {
		struct psf2_header *psfhdr;

		psfhdr = (struct psf2_header *) &inputbuf[0];

		if (psfhdr->version > PSF2_MAXVERSION) {
			char *u = _("%s: Unsupported psf version (%d)\n");
			fprintf(stderr, u, progname, psfhdr->version);
			exit(EX_DATAERR);
		}
		fontlen = psfhdr->length;
		charsize = psfhdr->charsize;
		hastable = (psfhdr->flags & PSF2_HAS_UNICODE_TABLE);
		ftoffset = psfhdr->headersize;
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

	if (hastable && uclistheadsp) {
		char *inptr, *endptr;

		inptr = inputbuf + ftoffset + fontlen * charsize;
		endptr = inputbuf + inputlth;

		*uclistheadsp = xrealloc(*uclistheadsp,
			(fontpos0+fontlen)*sizeof(struct unicode_list));

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
writepsffont(FILE *ofil, char *fontbuf, int charsize, int fontlen,
	     int psftype, struct unicode_list *uclistheads) {
	int i;
	int utf8;
	int seqs = 0;

	if (uclistheads)
		seqs = has_sequences(uclistheads, fontlen);

	/* Output new font file */
	if ((fontlen != 256 && fontlen != 512) || psftype == 2) {
		struct psf2_header h;

		h.magic[0] = PSF2_MAGIC0;
		h.magic[1] = PSF2_MAGIC1;
		h.magic[2] = PSF2_MAGIC2;
		h.magic[3] = PSF2_MAGIC3;
		h.version = 0;
		h.headersize = sizeof(h);
		h.flags = 0;
		if (uclistheads != NULL)
			h.flags |= PSF2_HAS_UNICODE_TABLE;
		h.length = fontlen;
		h.charsize = charsize;
		h.width = 8;
		h.height = charsize;
		fwrite(&h, sizeof(h), 1, ofil);
		utf8 = 1;
	} else {
		struct psf1_header h;

		h.magic[0] = PSF1_MAGIC0;
		h.magic[1] = PSF1_MAGIC1;
		h.mode = 0;
		if (fontlen == 512)
			h.mode |= PSF1_MODE512;
		if (uclistheads != NULL)
			h.mode |= (seqs ? PSF1_MODEHASSEQ : PSF1_MODEHASTAB);
		h.charsize = charsize;
		fwrite(&h, sizeof(h), 1, ofil);
		utf8 = 0;
	}
	fwrite(fontbuf, charsize, fontlen, ofil);
	if (uclistheads != NULL) {
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
}

