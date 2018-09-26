/* psffontop.h */

#ifndef _PSFFONTOP_H
#define _PSFFONTOP_H

#include "contextP.h"

int appendunicode(struct kfont_ctx *ctx, FILE *fp, unsigned int uc, int utf8);
int appendseparator(struct kfont_ctx *ctx, FILE *fp, int seq, int utf8);

#endif /* _PSFFONTOP_H */
