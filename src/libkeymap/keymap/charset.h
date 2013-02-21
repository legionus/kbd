#ifndef LK_CHARSET_H
#define LK_CHARSET_H

#include <keymap/data.h>

void lk_list_charsets(FILE *f);
int lk_set_charset(struct keymap *kmap, const char *name);

#endif /* LK_CHARSET_H */
