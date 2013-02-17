#ifndef LK_PARSE_H
#define LK_PARSE_H

#include <keymap/data.h>
#include <keymap/findfile.h>

int lk_parse_keymap(struct keymap *kmap, lkfile_t *f);
int lk_loadkeys(struct keymap *kmap, int fd, int kbd_mode);

#endif /* LK_PARSE_H */
