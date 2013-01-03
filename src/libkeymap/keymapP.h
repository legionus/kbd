#ifndef LK_KEYMAP_PRIVATE_H
#define LK_KEYMAP_PRIVATE_H

#define U(x) ((x) ^ 0xf000)

int stack_push(struct keymap *kmap, lkfile_t *fp, void *scanner);
int stack_pop(struct keymap *kmap, void *scanner);

int do_constant(struct keymap *kmap);
char *mk_mapname(char modifier);

void outchar(FILE *fd, unsigned char c, int comma);
void dumpchar(FILE *fd, unsigned char c, int comma);

int addkey(struct keymap *kmap, int k_index, int k_table, int keycode);
int addfunc(struct keymap *kmap, struct kbsentry kbs);

#endif /* LK_KEYMAP_PRIVATE_H */
