// SPDX-License-Identifier: LGPL-2.0-or-later
#ifndef KBD_REALLOCARRAY_H
#define KBD_REALLOCARRAY_H

#include "config.h"

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

static inline void *
kbd_reallocarray(void *ptr, size_t nmemb, size_t size)
{
#ifdef HAVE_REALLOCARRAY
	return reallocarray(ptr, nmemb, size);
#else
	if (size && nmemb > SIZE_MAX / size) {
		errno = ENOMEM;
		return NULL;
	}

	return realloc(ptr, nmemb * size);
#endif
}

#endif /* KBD_REALLOCARRAY_H */
