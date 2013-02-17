#ifndef LK_LOAD_H
#define LK_LOAD_H

#include <keymap/data.h>

int lk_get_keymap(struct keymap *kmap, int console);
int lk_get_keys(struct keymap *kmap, int console);
int lk_get_funcs(struct keymap *kmap, int console);
int lk_get_diacrs(struct keymap *kmap, int console);

#endif /* LK_LOAD_H */
