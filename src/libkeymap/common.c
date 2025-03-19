#include "config.h"

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "keymap.h"
#include "contextP.h"

void
lk_log(struct lk_ctx *ctx, int priority,
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

static void KBD_ATTR_PRINTF(6, 0)
log_file(void *data,
         int priority KBD_ATTR_UNUSED,
         const char *file KBD_ATTR_UNUSED,
         const int line KBD_ATTR_UNUSED,
         const char *fn KBD_ATTR_UNUSED,
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
	fprintf(fp, "libkeymap: %s %s:%d %s: ", priname, file, line, fn);
#endif
	vfprintf(fp, format, args);
	fprintf(fp, "\n");
}

int lk_set_log_fn(struct lk_ctx *ctx, lk_logger_t log_fn, const void *data)
{
	if (!ctx)
		return -1;

	ctx->log_fn   = log_fn;
	ctx->log_data = (void *)data;

	return 0;
}

lk_logger_t lk_get_log_fn(struct lk_ctx *ctx)
{
	if (!ctx)
		return NULL;

	return ctx->log_fn;
}

void *lk_get_log_data(struct lk_ctx *ctx)
{
	if (!ctx)
		return NULL;

	return ctx->log_data;
}

int lk_get_log_priority(struct lk_ctx *ctx)
{
	if (!ctx)
		return -1;

	return ctx->log_priority;
}

int lk_set_log_priority(struct lk_ctx *ctx, int priority)
{
	if (!ctx)
		return -1;

	ctx->log_priority = priority;
	return 0;
}

lk_flags
lk_get_parser_flags(struct lk_ctx *ctx)
{
	if (!ctx)
		return -1;

	return ctx->flags;
}

int lk_set_parser_flags(struct lk_ctx *ctx, lk_flags flags)
{
	if (!ctx)
		return -1;

	ctx->flags = flags;
	return 0;
}

lk_keywords lk_get_keywords(struct lk_ctx *ctx)
{
	if (!ctx)
		return -1;

	return ctx->keywords;
}

int lk_set_keywords(struct lk_ctx *ctx, lk_keywords keywords)
{
	if (!ctx)
		return -1;

	ctx->keywords = keywords;
	return 0;
}

static int
init_array(struct lk_ctx *ctx, struct lk_array **arr, ssize_t size)
{
	int rc;
	void *ptr;

	ptr = malloc(sizeof(struct lk_array));
	if (!ptr) {
		ERR(ctx, _("out of memory"));
		return -1;
	}

	rc = lk_array_init(ptr, size, 0);
	if (rc < 0) {
		ERR(ctx, _("unable to initialize array: %s"), strerror(rc));
		free(ptr);
		return -1;
	}

	*arr = ptr;

	return 0;
}

struct lk_ctx *
lk_init(void)
{
	struct lk_ctx *ctx;

	ctx = malloc(sizeof(struct lk_ctx));
	if (!ctx)
		return NULL;

	memset(ctx, 0, sizeof(struct lk_ctx));

	lk_set_log_fn(ctx, log_file, stderr);
	lk_set_log_priority(ctx, LOG_ERR);

	int ret = 0;

	ret = ret ?: init_array(ctx, &ctx->keymap, sizeof(void *));
	ret = ret ?: init_array(ctx, &ctx->func_table, sizeof(void *));
	ret = ret ?: init_array(ctx, &ctx->accent_table, sizeof(void *));
	ret = ret ?: init_array(ctx, &ctx->key_constant, sizeof(char));
	ret = ret ?: init_array(ctx, &ctx->key_line, sizeof(int));

	if (ret) {
		lk_free(ctx);
		return NULL;
	}

	ctx->kbdfile_ctx = kbdfile_context_new();

	if (ctx->kbdfile_ctx == NULL) {
		lk_free(ctx);
		return NULL;
	}

	return ctx;
}

enum ctx_array_type {
	FREE_CTX_KEYMAP,
	FREE_CTX_FUNC_TABLE,
	FREE_CTX_ACCENT_TABLE,
};

static void *free_ctx_array(struct lk_array *arr, enum ctx_array_type array_type)
{
	if (!arr)
		return NULL;

	for (int i = 0; i < arr->total; i++) {
		void *ptr = lk_array_get_ptr(arr, i);

		if (!ptr)
			continue;

		switch(array_type) {
			case FREE_CTX_KEYMAP:
				lk_array_free(ptr);
				break;
			case FREE_CTX_FUNC_TABLE:
			case FREE_CTX_ACCENT_TABLE:
				break;
		}

		free(ptr);
	}

	lk_array_free(arr);
	free(arr);

	return NULL;
}

int lk_free(struct lk_ctx *ctx)
{
	if (!ctx)
		return -1;

	ctx->keymap       = free_ctx_array(ctx->keymap, FREE_CTX_KEYMAP);
	ctx->func_table   = free_ctx_array(ctx->func_table, FREE_CTX_FUNC_TABLE);
	ctx->accent_table = free_ctx_array(ctx->accent_table, FREE_CTX_ACCENT_TABLE);

	if (ctx->key_constant) {
		lk_array_free(ctx->key_constant);
		free(ctx->key_constant);
		ctx->key_constant = NULL;
	}

	if (ctx->key_line) {
		lk_array_free(ctx->key_line);
		free(ctx->key_line);
		ctx->key_line = NULL;
	}

	if (ctx->kbdfile_ctx != NULL)
		ctx->kbdfile_ctx = kbdfile_context_free(ctx->kbdfile_ctx);

	free(ctx);

	return 0;
}
