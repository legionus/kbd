#include <stdio.h>
#include <syslog.h>

#include "libcommon.h"
#include "context.h"
#include "paths.h"

/* search for the map file in these directories (with trailing /) */
static const char *const mapdirpath[]  = {
	"",
	DATADIR "/" TRANSDIR "/",
	NULL
};

static const char *const mapsuffixes[] = {
	"",
	".trans",
	"_to_uni.trans",
	".acm",
	NULL
};

/* search for the font in these directories (with trailing /) */
static const char *const fontdirpath[]  = {
	"",
	DATADIR "/" FONTDIR "/",
	NULL
};
static char const *const fontsuffixes[] = {
	"",
	".psfu",
	".psf",
	".cp",
	".fnt",
	NULL
};

static const char *const unidirpath[]  = {
	"",
	DATADIR "/" UNIMAPDIR "/",
	NULL
};
static const char *const unisuffixes[] = {
	"",
	".uni",
	".sfm",
	NULL
};

/* hide partial fonts a bit - loading a single one is a bad idea */
const char *const partfontdirpath[]  = {
	"",
	DATADIR "/" FONTDIR "/" PARTIALDIR "/",
	NULL
};
char const *const partfontsuffixes[] = {
	"",
	NULL
};

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

void
kfont_init(struct kfont_context *ctx)
{
	ctx->progname = get_progname();
	ctx->verbose = 0;
	ctx->log_fn = kfont_log_stderr;
	ctx->mapdirpath = mapdirpath;
	ctx->mapsuffixes = mapsuffixes;
	ctx->fontdirpath = fontdirpath;
	ctx->fontsuffixes = fontsuffixes;
	ctx->partfontdirpath = partfontdirpath;
	ctx->partfontsuffixes = partfontsuffixes;
	ctx->unidirpath = unidirpath;
	ctx->unisuffixes = unisuffixes;
}
