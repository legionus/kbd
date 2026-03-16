#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <linux/kd.h>
#include <syslog.h>

#include <kfont.h>
#include "kfontP.h"
#include "libcommon.h"

static unsigned int log_calls;
static int last_priority = -1;
static char last_message[128];
static unsigned int current_mode = KD_TEXT;

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
	if (log_calls != calls || last_priority != priority || strcmp(last_message, message) != 0)
		kbd_error(EXIT_FAILURE, 0, "unexpected captured log state");
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

static int
fake_ioctl_helper_paths(int fd, unsigned long req, void *arg)
{
	if (fd != 127)
		kbd_error(EXIT_FAILURE, 0, "unexpected fd: %d", fd);

	if (req == KDGETMODE) {
		*(unsigned int *) arg = current_mode;
		return 0;
	}

	if (req == KDFONTOP) {
		struct console_font_op *cfo = arg;

		if (cfo->op == KD_FONT_OP_GET) {
			cfo->charcount = 512;
			cfo->width = 8;
			cfo->height = 16;
			if (cfo->data)
				memset(cfo->data, 0, cfo->charcount * 32U);
			return 0;
		}

		if (cfo->op == KD_FONT_OP_SET_DEFAULT)
			return 0;
	}

	return -1;
}

static void
test_context_logging_and_options(void)
{
	struct kfont_context *ctx;
	char path[] = "/tmp/libkfont-test24-stderr-XXXXXX";
	int fd, saved_stderr;
	FILE *fp;
	char buf[256];
	size_t nread;

	if (kfont_init("libkfont-test24", &ctx) != 0)
		kbd_error(EXIT_FAILURE, 0, "unable to create kfont context");

	if (kfont_get_verbosity(ctx) != 0 || ctx->log_fn != log_stderr)
		kbd_error(EXIT_FAILURE, 0, "unexpected default context state");

	kfont_set_option(ctx, kfont_force);
	kfont_set_option(ctx, kfont_double_size);
	kfont_unset_option(ctx, kfont_force);
	if (ctx->options != (1U << kfont_double_size))
		kbd_error(EXIT_FAILURE, 0, "unexpected option mask");

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
	if (saved_stderr < 0 || dup2(fd, STDERR_FILENO) < 0)
		kbd_error(EXIT_FAILURE, 0, "stderr redirection failed");

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
		kbd_error(EXIT_FAILURE, 0, "unexpected warning stderr output");

	kfont_free(ctx);
}

static void
test_kdfontop_helpers(void)
{
	struct kfont_context *ctx;
	struct kfont_ops ops;
	unsigned char fontbuf[32];

	if (kfont_init("libkfont-test24", &ctx) != 0)
		kbd_error(EXIT_FAILURE, 0, "unable to create kfont context");

	kfont_set_logger(ctx, NULL);
	ops = ctx->ops;
	ops.ioctl_fn = fake_ioctl_helper_paths;
	kfont_set_ops(ctx, &ops);

	if (kfont_get_fontsize(ctx, 127) != 512)
		kbd_error(EXIT_FAILURE, 0, "unexpected font size");

	if (!kfont_is_font_console(ctx, 127))
		kbd_error(EXIT_FAILURE, 0, "text console was not detected");

	if (kfont_restore_font(ctx, 127) != 0)
		kbd_error(EXIT_FAILURE, 0, "unable to restore default font");

	memset(fontbuf, 0, sizeof(fontbuf));
	fontbuf[2] = 1;
	if (font_charheight(fontbuf, 1, 8) != 3)
		kbd_error(EXIT_FAILURE, 0, "unexpected font height");

	current_mode = KD_GRAPHICS;
	if (kfont_is_font_console(ctx, 127))
		kbd_error(EXIT_FAILURE, 0, "graphics console unexpectedly accepted");
	if (kfont_restore_font(ctx, 127) == 0)
		kbd_error(EXIT_FAILURE, 0, "restoring font unexpectedly succeeded in graphics mode");

	current_mode = KD_TEXT;
	kfont_free(ctx);
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	test_context_logging_and_options();
	test_kdfontop_helpers();
	return EXIT_SUCCESS;
}
