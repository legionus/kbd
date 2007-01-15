typedef unsigned int unicode;

struct unicode_seq {
	struct unicode_seq *next;
	struct unicode_seq *prev;
	unicode uc;
};

struct unicode_list {
	struct unicode_list *next;
	struct unicode_list *prev;
	struct unicode_seq *seq;
};

extern int readpsffont(FILE *fontf, char **allbufp, int *allszp,
		       char **fontbufp, int *fontszp,
		       int *fontlenp, int fontpos0,
		       struct unicode_list **uclistheadsp);

extern void writepsffont(FILE *ofil, char *fontbuf,
			 int charsize, int fontlen, int psftype,
			 struct unicode_list *uclistheads);

extern void appendunicode(FILE *fp, unsigned int uc, int utf8);
extern void appendseparator(FILE *fp, int seq, int utf8);
