#ifndef _CONTEXT_H_
#define _CONTEXT_H_

#ifndef __GNUC__
#undef  __attribute__
#define __attribute__(x) /*NOTHING*/
#endif

struct kfont_context;

typedef void (*logger_t)(struct kfont_context *, int, const char *, int,
		const char *, const char *, va_list)
	__attribute__((nonnull(1)))
	__attribute__((format(printf, 6, 0)));

struct kfont_context {
	const char *progname;
	logger_t log_fn;
};

void logger(struct kfont_context *ctx, int priority, const char *file, int line,
		const char *fn, const char *fmt, ...)
	__attribute__((format(printf, 6, 7)))
	__attribute__((nonnull(1)));

#include <syslog.h>

#define DBG(ctx,  arg...) logger(ctx, LOG_DEBUG,   __FILE__, __LINE__, __func__, ##arg)
#define INFO(ctx, arg...) logger(ctx, LOG_INFO,    __FILE__, __LINE__, __func__, ##arg)
#define WARN(ctx, arg...) logger(ctx, LOG_WARNING, __FILE__, __LINE__, __func__, ##arg)
#define ERR(ctx,  arg...) logger(ctx, LOG_ERR,     __FILE__, __LINE__, __func__, ##arg)

#include <stdarg.h>

void log_stderr(struct kfont_context *ctx, int priority, const char *file,
		const int line, const char *fn, const char *format, va_list args)
	__attribute__((format(printf, 6, 0)));

#endif /* _CONTEXT_H_ */
