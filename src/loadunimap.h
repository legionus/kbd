/* loadunimap.h */
void saveunicodemap(int fd, char *oufil);	/* save humanly readable */
void loadunicodemap(int fd, char *ufil);
void appendunicodemap(int fd, FILE *fp, int ct, int utf8);
