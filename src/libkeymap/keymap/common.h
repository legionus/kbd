#ifndef LK_COMMON_H
#define LK_COMMON_H

#include <keymap/data.h>

int lk_init(struct keymap *km);
int lk_free(struct keymap *kmap);

int lk_get_log_priority(struct keymap *kmap);
int lk_set_log_priority(struct keymap *kmap, int priority);

int lk_set_log_fn(struct keymap *kmap,
		void (*log_fn)(void *data, int priority,
		               const char *file, int line, const char *fn,
		               const char *format, va_list args),
		const void *data);

#endif /* LK_COMMON_H */
