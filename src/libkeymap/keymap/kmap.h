#ifndef LK_KMAP_H
#define LK_KMAP_H

#include <keymap/data.h>
#include <keymap/findfile.h>

int lk_add_map(struct keymap *kmap, int i);

int lk_add_key(struct keymap *kmap, int k_index, int k_table, int keycode);
int lk_remove_key(struct keymap *kmap, int k_index, int k_table);
int lk_get_key(struct keymap *kmap, int k_table, int k_index);


int lk_get_func(struct keymap *kmap, struct kbsentry *kbs);
int lk_add_func(struct keymap *kmap, struct kbsentry kbs);

int lk_add_diacr(struct keymap *kmap, unsigned int diacr, unsigned int base, unsigned int res);
int lk_add_compose(struct keymap *kmap, unsigned int diacr, unsigned int base, unsigned int res);

int lk_add_constants(struct keymap *kmap);

int lk_parse_keymap(struct keymap *kmap, lkfile_t *f);
int lk_load_keymap(struct keymap *kmap, int fd, int kbd_mode);

#endif /* LK_KMAP_H */
