#ifndef LK_KERNEL_H
#define LK_KERNEL_H

#include <keymap/data.h>

int lk_kernel_keymap(struct keymap *kmap, int console);
int lk_kernel_keys(struct keymap *kmap, int console);
int lk_kernel_funcs(struct keymap *kmap, int console);
int lk_kernel_diacrs(struct keymap *kmap, int console);

#endif /* LK_KERNEL_H */
