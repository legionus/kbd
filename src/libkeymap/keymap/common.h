#ifndef LK_COMMON_H
#define LK_COMMON_H

#include <keymap/data.h>

int lk_init(struct keymap *km);
void lk_free(struct keymap *kmap);

#endif /* LK_COMMON_H */
