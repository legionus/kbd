#include <stdio.h>
#include <syslog.h>

#include "context.h"

void
kfont_logger(struct kfont_context *ctx, int priority, const char *file,
		int line, const char *fn,
		const char *fmt, ...)
{
	va_list args;
	if (ctx->log_fn == NULL)
		return;
	va_start(args, fmt);
	ctx->log_fn(ctx, priority, file, line, fn, fmt, args);
	va_end(args);
}


void
kfont_log_stderr(struct kfont_context *ctx, int priority, const char *file,
		const int line, const char *fn,
		const char *format, va_list args)
{
	char buf[16];
	const char *priname;

	switch (priority) {
		case LOG_EMERG:   priname = "EMERGENCY"; break;
		case LOG_ALERT:   priname = "ALERT";     break;
		case LOG_CRIT:    priname = "CRITICAL";  break;
		case LOG_ERR:     priname = "ERROR";     break;
		case LOG_WARNING: priname = "WARNING";   break;
		case LOG_NOTICE:  priname = "NOTICE";    break;
		case LOG_INFO:    priname = "INFO";      break;
		case LOG_DEBUG:   priname = "DEBUG";     break;
		default:
			snprintf(buf, sizeof(buf), "L:%d", priority);
			priname = buf;
	}

	fprintf(stderr, "%s: %s %s:%d %s: ", ctx->progname, priname, file, line, fn);
	vfprintf(stderr, format, args);
	fprintf(stderr, "\n");

	fflush(stderr);
}
