#ifndef _PARSEP_H_
#define _PARSEP_H_

#include <linux/kd.h>
#include <linux/keyboard.h>

#define MAX_PARSER_STRING 512

struct strdata {
	unsigned int len;
	unsigned char data[MAX_PARSER_STRING];
};

#define U(x) ((x) ^ 0xf000)

int stack_push(struct keymap *kmap, lkfile_t *fp, void *scanner);
int stack_pop(struct keymap *kmap, void *scanner);

int do_constant(struct keymap *kmap);
char *mk_mapname(char modifier);

void outchar(FILE *fd, unsigned char c, int comma);
void dumpchar(FILE *fd, unsigned char c, int comma);

int addkey(struct keymap *kmap, int k_index, int k_table, int keycode);
int addfunc(struct keymap *kmap, struct kbsentry kbs);
int compose(struct keymap *kmap, unsigned int diacr, unsigned int base, unsigned int res);

#endif /* _PARSEP_H_ */
