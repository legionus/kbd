// SPDX-License-Identifier: GPL-2.0-or-later

#include "config.h"

#include <unistd.h>
#include <stdint.h>
#include <stdarg.h>
#include <errno.h>
#include <dlfcn.h>

#include "elf-note.h"

static int dlsym_manyv(void *dl, va_list ap)
{
	void (**fn)(void);

	while ((fn = va_arg(ap, typeof(fn)))) {
		const char *symbol;

		symbol = va_arg(ap, typeof(symbol));
		*fn = dlsym(dl, symbol);
		if (!*fn)
			return -ENXIO;
	}

	return 0;
}

int dlsym_many(void **dlp, const char *filename, ...)
{
	va_list ap;
	void *dl;
	int r;

	if (*dlp)
		return 0;

	dl = dlopen(filename, RTLD_LAZY);
	if (!dl)
		return -ENOENT;

	va_start(ap, filename);
	r = dlsym_manyv(dl, ap);
	va_end(ap);

	if (r < 0) {
		dlclose(dl);
		return r;
	}

	*dlp = dl;

	return 1;
}
