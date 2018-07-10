/* loadunimap.h */

#ifndef _LOADUNIMAP_H
#define _LOADUNIMAP_H

int saveunicodemap(int fd, char *oufil); /* save humanly readable */
int loadunicodemap(int fd, const char *ufil, const char *const *unidirpath, const char *const *unisuffixes);
int appendunicodemap(int fd, FILE *fp, int ct, int utf8);

#endif /* _LOADUNIMAP_H */
