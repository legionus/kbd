#ifndef LK_LOGGING_H
#define LK_LOGGING_H

#include <syslog.h>
#include <keymap/context.h>

void lk_log(struct lk_ctx *ctx, int priority,
            const char *file, int line, const char *fn,
            const char *fmt, ...);

#define lk_log_cond(ctx, level, arg...) \
	do { \
		if (ctx->log_priority >= level) \
			lk_log(ctx, level, __FILE__, __LINE__, __func__, ## arg);\
	} while (0)

#define DBG(ctx, arg...)  lk_log_cond(ctx, LOG_DEBUG,   ## arg)
#define INFO(ctx, arg...) lk_log_cond(ctx, LOG_INFO,    ## arg)
#define WARN(ctx, arg...) lk_log_cond(ctx, LOG_WARNING, ## arg)
#define ERR(ctx, arg...)  lk_log_cond(ctx, LOG_ERR,     ## arg)

#endif /* LK_LOGGING_H */
