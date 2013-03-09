#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "kbd.h"
#include "keymap.h"

static void attr_fmt45
lkmessage(const char *file attr_unused, int line attr_unused, const char *fn attr_unused,
          const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
#ifdef DEBUG
	fprintf(stdout, "%s(%d): %s: ", file, line, fn);
#endif
	vfprintf(stdout, fmt, ap);
	fprintf(stdout, "\n");
	va_end(ap);
}

static void attr_fmt45
lkerror(const char *file attr_unused, int line attr_unused, const char *fn attr_unused,
        const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	va_end(ap);
}

int
lk_init(struct keymap *kmap)
{
	if (!kmap)
		return -1;

	memset(kmap, 0, sizeof(struct keymap));

	kmap->verbose     = LOG_NORMAL;
	kmap->log_message = lkmessage;
	kmap->log_error   = lkerror;

	return 0;
}


void
lk_free(struct keymap *kmap)
{
	int i;

	if (!kmap)
		return;

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
}
