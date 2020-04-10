/* psffontop.h */

#ifndef _PSFFONTOP_H
#define _PSFFONTOP_H

#include <stdint.h>
#include "unicode.h"

/* Maximum font size that we try to handle */
#define MAXFONTSIZE 65536

/**
 * readpsffont reads a PSF font.
 *
 * The font is read either from a file (when fontf is non-NULL) or from memory
 * (namely from @p *allbufp of size @p *allszp). In the former case, if
 * @p allbufp is non-NULL, a pointer to the entire fontfile contents (possibly
 * read from pipe) is returned in @p *allbufp, and the size in @p allszp, where
 * this buffer was allocated using malloc().
 *
 * In @p fontbufp, @p fontszp the subinterval of @p allbufp containing the font
 * data is given.
 *
 * The font width is stored in @p fontwidthp.
 *
 * The number of glyphs is stored in @p fontlenp.
 *
 * The unicode table is stored in @p uclistheadsp (when non-NULL), with
 * fontpositions counted from @p fontpos0 (so that calling this several times
 * can achieve font merging).
 *
 * @returns >= 0 on success and -1 on failure. Failure means that the font was
 * not psf (but has been read). > 0 means that the Unicode table contains
 * sequences.
 */
extern int readpsffont(FILE *fontf, unsigned char **allbufp, unsigned int *allszp,
                       unsigned char **fontbufp, unsigned int *fontszp,
                       unsigned int *fontwidthp, unsigned int *fontlenp, unsigned int fontpos0,
                       struct unicode_list **uclistheadsp);

extern int writepsffont(FILE *ofil, unsigned char *fontbuf,
                        unsigned int width, unsigned int height, unsigned int fontlen, int psftype,
                        struct unicode_list *uclistheads);

#define WPSFH_HASTAB 1
#define WPSFH_HASSEQ 2
extern void writepsffontheader(FILE *ofil,
                               unsigned int width, unsigned int height, unsigned int fontlen,
                               int *psftype, int flags);

extern void appendunicode(FILE *fp, unicode uc, int utf8);
extern void appendseparator(FILE *fp, int seq, int utf8);

#endif /* _PSFFONTOP_H */
