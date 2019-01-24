#include "config.h"

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include <kbdfile.h>

#include "kfont.h"

#include "libcommon.h"
#include "contextP.h"

#define UNIMAPDIR "unimaps"
#define TRANSDIR "consoletrans"
#define FONTDIR "consolefonts"
#define PARTIALDIR "partialfonts"

/* search for the map file in these directories (with trailing /) */
const char *const mapdirs[]     = { "", DATADIR "/" TRANSDIR "/", 0 };
const char *const mapsuffixes[] = { "", ".trans", "_to_uni.trans", ".acm", 0 };

/* search for the font in these directories (with trailing /) */
const char *const fontdirs[]     = { "", DATADIR "/" FONTDIR "/", 0 };
const char *const fontsuffixes[] = { "", ".psfu", ".psf", ".cp", ".fnt", 0 };

/* hide partial fonts a bit - loading a single one is a bad idea */
const char *const partfontdirs[]     = { "", DATADIR "/" FONTDIR "/" PARTIALDIR "/", 0 };
const char *const partfontsuffixes[] = { "", 0 };

static const char *const unidirs[]     = { "", DATADIR "/" UNIMAPDIR "/", 0 };
static const char *const unisuffixes[] = { "", ".uni", ".sfm", 0 };

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

	ctx->log_fn = log_fn;
	ctx->log_data = (void *) data;

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

	ctx->log_data = (void *) data;
	return 0;
}

int
kfont_get_log_priority(struct kfont_ctx *ctx)
{
	if (!ctx)
		return -1;

	return ctx->log_priority;
}

int
kfont_set_log_priority(struct kfont_ctx *ctx, int priority)
{
	if (!ctx)
		return -1;

	ctx->log_priority = priority;
	return 0;
}

kfont_flags
kfont_get_flags(struct kfont_ctx *ctx)
{
	if (!ctx)
		return 0;

	return ctx->flags;
}

int
kfont_set_flags(struct kfont_ctx *ctx, kfont_flags flags)
{
	if (!ctx)
		return -1;

	ctx->flags = flags;
	return 0;
}

int
kfont_set_fontdirs(struct kfont_ctx *ctx, char **dirs, char **suffixes)
{
	if (!ctx)
		return -1;

	ctx->fontdirs     = dirs;
	ctx->fontsuffixes = suffixes;

	return 0;
}

int
kfont_set_partfontdirs(struct kfont_ctx *ctx, char **dirs, char **suffixes)
{
	if (!ctx)
		return -1;

	ctx->partfontdirs     = dirs;
	ctx->partfontsuffixes = suffixes;

	return 0;
}

int
kfont_set_mapdirs(struct kfont_ctx *ctx, char **dirs, char **suffixes)
{
	if (!ctx)
		return -1;

	ctx->mapdirs     = dirs;
	ctx->mapsuffixes = suffixes;

	return 0;
}

int
kfont_set_unidirs(struct kfont_ctx *ctx, char **dirs, char **suffixes)
{
	if (!ctx)
		return -1;

	ctx->unidirs     = dirs;
	ctx->unisuffixes = suffixes;

	return 0;
}

int
kfont_set_console(struct kfont_ctx *ctx, int fd)
{
	if (!ctx)
		return -1;

	ctx->consolefd = fd;

	return 0;
}

struct kfont_ctx *
kfont_context_new(void)
{
	struct kfont_ctx *ctx;

	ctx = calloc(1, sizeof(struct kfont_ctx));
	if (!ctx)
		return NULL;

	if ((ctx->kbdfile_ctx = kbdfile_context_new()) == NULL) {
		free(ctx);
		return NULL;
	}

	ctx->consolefd = -1;

	ctx->fontdirs     = (char **) fontdirs;
	ctx->fontsuffixes = (char **) fontsuffixes;

	ctx->partfontdirs     = (char **) partfontdirs;
	ctx->partfontsuffixes = (char **) partfontsuffixes;

	ctx->mapdirs     = (char **) mapdirs;
	ctx->mapsuffixes = (char **) mapsuffixes;

	ctx->unidirs     = (char **) unidirs;
	ctx->unisuffixes = (char **) unisuffixes;

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
