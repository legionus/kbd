#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <syslog.h>

#include "kfontP.h"
#include "libcommon.h"

static unsigned int log_calls;
static int last_priority = -1;
static char last_message[128];

static void
capture_logger(struct kfont_context *ctx KBD_ATTR_UNUSED, int priority,
	       const char *file KBD_ATTR_UNUSED, int line KBD_ATTR_UNUSED,
	       const char *fn KBD_ATTR_UNUSED, const char *format, va_list args)
{
	log_calls++;
	last_priority = priority;
	vsnprintf(last_message, sizeof(last_message), format, args);
}

static void
expect_logged(unsigned int calls, int priority, const char *message)
{
	if (log_calls != calls)
		kbd_error(EXIT_FAILURE, 0, "unexpected log call count: %u", log_calls);

	if (last_priority != priority)
		kbd_error(EXIT_FAILURE, 0, "unexpected log priority: %d", last_priority);

	if (strcmp(last_message, message) != 0)
		kbd_error(EXIT_FAILURE, 0, "unexpected log message: %s", last_message);
}

static void KBD_ATTR_PRINTF(6, 7)
forward_to_log_stderr(struct kfont_context *ctx, int priority, const char *file,
		      int line, const char *fn, const char *format, ...)
{
	va_list args;

	va_start(args, format);
	log_stderr(ctx, priority, file, line, fn, format, args);
	va_end(args);
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct kfont_context *ctx;
	char path[] = "/tmp/libkfont-test24-stderr-XXXXXX";
	int fd;
	int saved_stderr;
	FILE *fp;
	char buf[256];
	size_t nread;

	if (kfont_init("libkfont-test24", &ctx) != 0)
		kbd_error(EXIT_FAILURE, 0, "unable to create kfont context");

	if (kfont_get_verbosity(ctx) != 0)
		kbd_error(EXIT_FAILURE, 0, "unexpected default verbosity");

	if (ctx->log_fn != log_stderr)
		kbd_error(EXIT_FAILURE, 0, "unexpected default logger");

	kfont_set_option(ctx, kfont_force);
	kfont_set_option(ctx, kfont_double_size);
	if (ctx->options != ((1U << kfont_force) | (1U << kfont_double_size)))
		kbd_error(EXIT_FAILURE, 0, "unexpected options after setting them");

	kfont_unset_option(ctx, kfont_force);
	if (ctx->options != (1U << kfont_double_size))
		kbd_error(EXIT_FAILURE, 0, "unexpected options after unsetting one");

	kfont_set_logger(ctx, capture_logger);

	KFONT_INFO(ctx, "ignored info");
	if (log_calls != 0)
		kbd_error(EXIT_FAILURE, 0, "verbosity 0 should suppress INFO logs");

	KFONT_ERR(ctx, "first error");
	expect_logged(1, LOG_ERR, "first error");

	kfont_inc_verbosity(ctx);
	KFONT_INFO(ctx, "second info");
	expect_logged(2, LOG_INFO, "second info");

	kfont_inc_verbosity(ctx);
	KFONT_DBG(ctx, "third debug");
	expect_logged(3, LOG_DEBUG, "third debug");

	fd = mkstemp(path);
	if (fd < 0)
		kbd_error(EXIT_FAILURE, 0, "mkstemp failed");

	saved_stderr = dup(STDERR_FILENO);
	if (saved_stderr < 0)
		kbd_error(EXIT_FAILURE, 0, "dup failed");

	if (dup2(fd, STDERR_FILENO) < 0)
		kbd_error(EXIT_FAILURE, 0, "dup2 failed");

	kfont_set_logger(ctx, log_stderr);
	forward_to_log_stderr(ctx, LOG_WARNING, "file.c", 12, "func", "warn %d", 7);
	close(fd);

	dup2(saved_stderr, STDERR_FILENO);
	close(saved_stderr);

	fp = fopen(path, "r");
	if (!fp)
		kbd_error(EXIT_FAILURE, 0, "unable to read stderr capture");

	nread = fread(buf, 1, sizeof(buf) - 1, fp);
	buf[nread] = '\0';
	fclose(fp);
	unlink(path);

	if (!strstr(buf, "libkfont-test24: WARNING file.c:12 func: warn 7\n"))
		kbd_error(EXIT_FAILURE, 0, "unexpected warning stderr output: %s", buf);

	kfont_free(ctx);

	return EXIT_SUCCESS;
}
