#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "kbd.h"
#include "keymap.h"

void __attribute__ ((format (printf, 6, 7)))
lk_log(struct keymap *kmap, int priority,
       const char *file, int line, const char *fn,
       const char *fmt, ...)
{
	va_list args;
	if (kmap->log_fn == NULL)
		return;
	va_start(args, fmt);
	kmap->log_fn(kmap->log_data, priority, file, line, fn, fmt, args);
	va_end(args);
}

#ifndef DEBUG
#   define log_unused __attribute__ ((unused))
#else
#   define log_unused
#endif

static void
log_file(void *data,
         int priority     log_unused,
         const char *file log_unused,
         const int line   log_unused,
         const char *fn   log_unused,
         const char *format, va_list args)
{
	FILE *fp = data;
#ifdef DEBUG
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
	fprintf(fp, "libkeymap: %s %s:%d %s: ", priname, file, line, fn);
#endif
	vfprintf(fp, format, args);
	fprintf(fp, "\n");
}

#undef log_unused

int
lk_init(struct keymap *kmap)
{
	if (!kmap)
		return -1;

	memset(kmap, 0, sizeof(struct keymap));

	kmap->log_fn       = log_file;
	kmap->log_data     = stderr;
	kmap->log_priority = LOG_ERR;

	return 0;
}


int
lk_free(struct keymap *kmap)
{
	int i;

	if (!kmap)
		return -1;

	for (i = 0; i < MAX_NR_KEYMAPS; i++) {
		if (kmap->keymap_was_set[i] != NULL)
			free(kmap->keymap_was_set[i]);
		if (kmap->key_map[i] != NULL)
			free(kmap->key_map[i]);
	}

	for (i = 0; i < MAX_NR_FUNC; i++) {
		if (kmap->func_table[i] != NULL)
			free(kmap->func_table[i]);
	}

	return 0;
}
