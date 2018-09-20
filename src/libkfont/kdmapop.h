#ifndef _KDMAPOP_H
#define _KDMAPOP_H

#include "kfont.h"

int getscrnmap(struct kfont_ctx *ctx, int fd, char *map);
int loadscrnmap(struct kfont_ctx *ctx, int fd, char *map);
int getuniscrnmap(struct kfont_ctx *ctx, int fd, unsigned short *map);
int loaduniscrnmap(struct kfont_ctx *ctx, int fd, unsigned short *map);
int getunimap(struct kfont_ctx *ctx, int fd, struct unimapdesc *ud);
int loadunimap(struct kfont_ctx *ctx, int fd, struct unimapinit *ui, struct unimapdesc *ud);

#endif /* _KDMAPOP_H */
