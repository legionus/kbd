#ifndef _KFONT_CONTEXT_H_
#define _KFONT_CONTEXT_H_

#ifndef __GNUC__
#undef  __attribute__
#define __attribute__(x) /*NOTHING*/
#endif

struct kfont_context;

typedef void (*kfont_logger_t)(struct kfont_context *, int, const char *, int,
		const char *, const char *, va_list)
	__attribute__((nonnull(1)))
	__attribute__((format(printf, 6, 0)));

struct kfont_context {
	const char *progname;
	kfont_logger_t log_fn;
};

void kfont_logger(struct kfont_context *ctx, int priority, const char *file,
		int line, const char *fn, const char *fmt, ...)
	__attribute__((format(printf, 6, 7)))
	__attribute__((nonnull(1)));

#include <syslog.h>

#define KFONT_DBG(ctx,  arg...) kfont_logger(ctx, LOG_DEBUG,   __FILE__, __LINE__, __func__, ##arg)
#define KFONT_INFO(ctx, arg...) kfont_logger(ctx, LOG_INFO,    __FILE__, __LINE__, __func__, ##arg)
#define KFONT_WARN(ctx, arg...) kfont_logger(ctx, LOG_WARNING, __FILE__, __LINE__, __func__, ##arg)
#define KFONT_ERR(ctx,  arg...) kfont_logger(ctx, LOG_ERR,     __FILE__, __LINE__, __func__, ##arg)

#include <stdarg.h>

void kfont_log_stderr(struct kfont_context *ctx, int priority, const char *file,
		const int line, const char *fn, const char *format, va_list args)
	__attribute__((format(printf, 6, 0)));

#endif /* _KFONT_CONTEXT_H_ */
