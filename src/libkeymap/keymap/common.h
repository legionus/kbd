#ifndef LK_COMMON_H
#define LK_COMMON_H

#include <keymap/context.h>

int lk_init(struct lk_ctx *km);
int lk_free(struct lk_ctx *ctx);

int lk_get_log_priority(struct lk_ctx *ctx);
int lk_set_log_priority(struct lk_ctx *ctx, int priority);

int lk_set_log_fn(struct lk_ctx *ctx,
		void (*log_fn)(void *data, int priority,
		               const char *file, int line, const char *fn,
		               const char *format, va_list args),
		const void *data);

#endif /* LK_COMMON_H */
