#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>

#include <kbd/keymap/array.h>
#include <kbd/reallocarray.h>

static inline char *
array_item_ptr(struct lk_array *a, size_t i)
{
	if (!a || !a->array || !a->memb || i >= a->total || i > SIZE_MAX / a->memb) {
		errno = EINVAL;
		return NULL;
	}

	return (char *) a->array + (a->memb * i);
}

int
lk_array_init(struct lk_array *a, size_t memb, size_t size)
{
	if (!a) {
		errno = EINVAL;
		return -EINVAL;
	}

	memset(a, 0, sizeof(struct lk_array));

	if (!memb || size > SIZE_MAX / memb) {
		errno = EINVAL;
		return -EINVAL;
	}

	if (size) {
		a->array = calloc(size, memb);
		if (!a->array) {
			errno = ENOMEM;
			return -ENOMEM;
		}
	}

	a->memb  = memb;
	a->total = size;

	return 0;
}

int
lk_array_free(struct lk_array *a)
{
	if (!a) {
		errno = EINVAL;
		return -EINVAL;
	}
	free(a->array);
	memset(a, 0, sizeof(struct lk_array));
	return 0;
}

int
lk_array_empty(struct lk_array *a)
{
	if (!a || !a->memb || a->total > SIZE_MAX / a->memb) {
		errno = EINVAL;
		return -EINVAL;
	}

	if (a->array && a->total)
		memset(a->array, 0, a->memb * a->total);

	a->count = 0;

	return 0;
}

int
lk_array_exists(struct lk_array *a, size_t i)
{
	char *s;
	size_t k;

	s = array_item_ptr(a, i);
	if (!s)
		return 0;

	for (k = 0; k < a->memb; k++) {
		if (s[k] != 0)
			return 1;
	}

	return 0;
}

void *
lk_array_get(struct lk_array *a, size_t i)
{
	return array_item_ptr(a, i);
}

void *
lk_array_get_ptr(struct lk_array *a, size_t i)
{
	void *ptr = array_item_ptr(a, i);
	if (!ptr)
		return NULL;

	return *(void **) ptr;
}

static int
array_resize(struct lk_array *a, size_t i)
{
	if (!a || !a->memb || a->total > SIZE_MAX / a->memb) {
		errno = EINVAL;
		return -EINVAL;
	}

	if (i >= a->total) {
		char *tmp;

		if (i == SIZE_MAX || (i + 1) > SIZE_MAX / a->memb) {
			errno = EINVAL;
			return -EINVAL;
		}

		tmp = kbd_reallocarray(a->array, i + 1, a->memb);
		if (!tmp) {
			errno = ENOMEM;
			return -ENOMEM;
		}

		memset(tmp + (a->memb * a->total), 0, a->memb * (i + 1 - a->total));

		a->array = tmp;
		a->total = i + 1;
	}
	return 0;
}

int
lk_array_set(struct lk_array *a, size_t i, const void *e)
{
	int ret;
	int existed_before, exists_after;

	if (!a || !e) {
		errno = EINVAL;
		return -EINVAL;
	}

	ret = array_resize(a, i);
	if (ret < 0)
		return ret;

	existed_before = lk_array_exists(a, i);
	memcpy((char *) a->array + (a->memb * i), e, a->memb);
	exists_after = lk_array_exists(a, i);

	if (!existed_before && exists_after)
		a->count++;
	else if (existed_before && !exists_after)
		a->count--;

	return 0;
}

int
lk_array_unset(struct lk_array *a, size_t i)
{
	char *s = array_item_ptr(a, i);
	if (!s)
		return -EINVAL;

	if (lk_array_exists(a, i)) {
		memset(s, 0, a->memb);
		a->count--;
	}

	return 0;
}

int
lk_array_append(struct lk_array *a, const void *e)
{
	int ret;

	if (!a || !e) {
		errno = EINVAL;
		return -EINVAL;
	}

	ret = array_resize(a, a->count);
	if (ret < 0)
		return ret;

	memcpy((char *) a->array + (a->memb * a->count), e, a->memb);
	a->count++;

	return 0;
}
