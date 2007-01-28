/* loadunimap.h */

#ifndef _LOADUNIMAP_H
#define _LOADUNIMAP_H

void saveunicodemap(int fd, char *oufil);	/* save humanly readable */
void loadunicodemap(int fd, char *ufil);
void appendunicodemap(int fd, FILE *fp, int ct, int utf8);

#endif 	/* _LOADUNIMAP_H */
