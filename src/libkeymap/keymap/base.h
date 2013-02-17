#ifndef LK_BASE_H
#define LK_BASE_H

#include <keymap/data.h>

int lk_init(struct keymap *km);
void lk_free(struct keymap *kmap);

#endif /* LK_BASE_H */
