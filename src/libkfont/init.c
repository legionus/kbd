#include "config.h"

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include <kbdfile.h>

#include "kfont.h"

#include "libcommon.h"
#include "contextP.h"

void __attribute__((format(printf, 6, 7)))
kfont_log(struct kfont_ctx *ctx, int priority,
       const char *file, int line, const char *fn,
       const char *fmt, ...)
{
	va_list args;
	if (ctx->log_fn == NULL)
		return;
	va_start(args, fmt);
	ctx->log_fn(ctx->log_data, priority, file, line, fn, fmt, args);
	va_end(args);
}

#ifndef DEBUG
#define log_unused __attribute__((unused))
#else
#define log_unused
#endif

static void __attribute__((format(printf, 6, 0)))
log_file(void *data,
         int priority log_unused,
         const char *file log_unused,
         const int line log_unused,
         const char *fn log_unused,
         const char *format, va_list args)
{
	FILE *fp = data;
#ifdef DEBUG
	char buf[16];
	const char *priname;

	switch (priority) {
		case LOG_EMERG:
			priname = "EMERGENCY";
			break;
		case LOG_ALERT:
			priname = "ALERT";
			break;
		case LOG_CRIT:
			priname = "CRITICAL";
			break;
		case LOG_ERR:
			priname = "ERROR";
			break;
		case LOG_WARNING:
			priname = "WARNING";
			break;
		case LOG_NOTICE:
			priname = "NOTICE";
			break;
		case LOG_INFO:
			priname = "INFO";
			break;
		case LOG_DEBUG:
			priname = "DEBUG";
			break;
		default:
			snprintf(buf, sizeof(buf), "L:%d", priority);
			priname = buf;
	}
	fprintf(fp, "libkfont: %s %s:%d %s: ", priname, file, line, fn);
#endif
	vfprintf(fp, format, args);
	fprintf(fp, "\n");
}

#undef log_unused

kfont_logger_t
kfont_get_log_fn(struct kfont_ctx *ctx)
{
	if (!ctx)
		return NULL;

	return ctx->log_fn;
}

int
kfont_set_log_fn(struct kfont_ctx *ctx, kfont_logger_t log_fn, const void *data)
{
	if (!ctx)
		return -1;

	ctx->log_fn   = log_fn;
	ctx->log_data = (void *)data;

	return 0;
}

void *
kfont_get_log_data(struct kfont_ctx *ctx)
{
	if (!ctx)
		return NULL;

	return ctx->log_data;
}

int
kfont_set_log_data(struct kfont_ctx *ctx, const void *data)
{
	if (!ctx)
		return -1;

	ctx->log_data = (void *)data;
	return 0;
}

int kfont_get_log_priority(struct kfont_ctx *ctx)
{
	if (!ctx)
		return -1;

	return ctx->log_priority;
}

int kfont_set_log_priority(struct kfont_ctx *ctx, int priority)
{
	if (!ctx)
		return -1;

	ctx->log_priority = priority;
	return 0;
}

struct kfont_ctx *
kfont_context_new(void)
{
	struct kfont_ctx *ctx;

	ctx = calloc(1, sizeof(struct kfont_ctx));
	if (!ctx)
		return NULL;

	if ((ctx->kbdfile_ctx = kbdfile_context_new()) == NULL)
		return NULL;

	kfont_set_log_fn(ctx, log_file, stderr);
	kfont_set_log_priority(ctx, LOG_ERR);

	return ctx;
}

void *
kfont_context_free(struct kfont_ctx *ctx)
{
	if (!ctx)
		return NULL;

	free(ctx);
	return NULL;
}
