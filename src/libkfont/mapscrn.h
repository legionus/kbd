#ifndef _MAPSCRN_H
#define _MAPSCRN_H

#include "kfont.h"

int kfont_saveoldmap(struct kfont_ctx *ctx, int fd, char *omfil);
int kfont_loadnewmap(struct kfont_ctx *ctx, int fd, char *mfil, const char *const *mapdirpath, const char *const *mapsuffixes);

#endif // _MAPSCRN_H
