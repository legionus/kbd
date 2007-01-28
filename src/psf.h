/* psf.h */

/*
 * Format of a psf font file:
 *
 * 1. The header
 * 2. The font
 * 3. Unicode information
 */

/*
 * Format of the Unicode information:
 *
 * For each font position <uc>*<seq>*<term>
 * where <uc> is a 2-byte little endian Unicode value,
 * <seq> = <ss><uc><uc>*, <ss> = psf1 ? 0xFFFE : 0xFE,
 * <term> = psf1 ? 0xFFFF : 0xFF.
 * and * denotes zero or more occurrences of the preceding item.
 *
 * Semantics:
 * The leading <uc>* part gives Unicode symbols that are all
 * represented by this font position. The following sequences
 * are sequences of Unicode symbols - probably a symbol
 * together with combining accents - also represented by
 * this font position.
 *
 * Example:
 * At the font position for a capital A-ring glyph, we
 * may have:
 *	00C5,212B,FFFE,0041,030A,FFFF
 * Some font positions may be described by sequences only,
 * namely when there is no precomposed Unicode value for the glyph.
 */

#ifndef _PSF_H
#define _PSF_H


#define PSF1_MAGIC0	0x36
#define PSF1_MAGIC1	0x04

#define PSF1_MODE512    0x01
#define PSF1_MODEHASTAB 0x02
#define PSF1_MODEHASSEQ 0x04
#define PSF1_MAXMODE    0x05

#define PSF1_SEPARATOR  0xFFFF
#define PSF1_STARTSEQ   0xFFFE

struct psf1_header {
	unsigned char magic[2];	    /* Magic number */
	unsigned char mode;	    /* PSF font mode */
	unsigned char charsize;	    /* Character size */
};

/*
 * Format and semantics of psf2 version 0 are as psf (with PSF_MAXMODE == 5).
 * However, this allows one to specify the length.
 * It turns out to be very useful to be able to work with fonts
 * with a few symbols or even only one (like the Euro), and
 * with very large fonts (like several thousand Unicode symbols
 * done in the same style).
 * Following hpa's suggestion, psf2 uses UTF-8 rather than UCS-2,
 * and has 32-bit magic 0x864ab572.
 * The integers here are little endian 4-byte integers.
 */

#define PSF2_MAGIC0     0x72
#define PSF2_MAGIC1     0xb5
#define PSF2_MAGIC2	0x4a
#define PSF2_MAGIC3	0x86

struct psf2_header {
	unsigned char magic[4];
	unsigned int version;
	unsigned int headersize;    /* offset of bitmaps in file */
	unsigned int flags;
	unsigned int length;	    /* number of glyphs */
	unsigned int charsize;	    /* number of bytes for each character */
	unsigned int height, width; /* max dimensions of glyphs */
	/* charsize = height * ((width + 7) / 8) */
};

/* bits used in flags */
#define PSF2_HAS_UNICODE_TABLE 0x01

/* max version recognized so far */
#define PSF2_MAXVERSION	0

/* UTF8 separators */
#define PSF2_SEPARATOR  0xFF
#define PSF2_STARTSEQ   0xFE


#define PSF1_MAGIC_OK(x)	((x)[0]==PSF1_MAGIC0 && (x)[1]==PSF1_MAGIC1)
#define PSF2_MAGIC_OK(x)	((x)[0]==PSF2_MAGIC0 && (x)[1]==PSF2_MAGIC1 \
				&& (x)[2]==PSF2_MAGIC2 && (x)[3]==PSF2_MAGIC3)


#endif	/* _PSF_H */
