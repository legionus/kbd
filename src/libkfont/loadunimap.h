/* loadunimap.h */

#ifndef _LOADUNIMAP_H
#define _LOADUNIMAP_H

#include "kfont.h"

int saveunicodemap(struct kfont_ctx *ctx, int fd, char *oufil); /* save humanly readable */
int loadunicodemap(struct kfont_ctx *ctx, int fd, const char *ufil, const char *const *unidirpath, const char *const *unisuffixes);
int appendunicodemap(struct kfont_ctx *ctx, int fd, FILE *fp, int ct, int utf8);

#endif /* _LOADUNIMAP_H */
