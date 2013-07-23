#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>

#include <keymap/array.h>

int
lk_array_init(struct lk_array *a, size_t memb, size_t size)
{
	if (!a)
		return -EINVAL;

	memset(a, 0, sizeof(struct lk_array));

	a->array = calloc(size, memb);
	a->memb  = memb;
	a->total = size;

	if (size && !a->array)
		return -ENOMEM;

	return 0;
}

int
lk_array_free(struct lk_array *a)
{
	if (!a)
		return -EINVAL;
	free(a->array);
	memset(a, 0, sizeof(struct lk_array));
	return 0;
}

int
lk_array_empty(struct lk_array *a)
{
	if (!a)
		return -EINVAL;

	memset(a->array, 0, (a->memb * a->total));
	a->count = 0;

	return 0;
}

int
lk_array_exists(struct lk_array *a, unsigned int i)
{
	char *s;
	size_t k;

	if (!a || i >= a->total) {
		return 0;
	}

	s = (char *) (a->array + (a->memb * i));

	for (k = 0; k < a->memb; k++) {
		if (s[k] != 0)
			return 1;
	}

	return 0;
}

void *
lk_array_get(struct lk_array *a, unsigned int i)
{
	if (!a || i >= a->total) {
		return NULL;
	}
	return a->array + (a->memb * i);
}

void *
lk_array_get_ptr(struct lk_array *a, unsigned int i)
{
	void **ptr;
	if (!a || i >= a->total) {
		return NULL;
	}
	ptr = a->array;
	return *(ptr + i);
}

static int
array_resize(struct lk_array *a, unsigned int i)
{
	if (!a)
		return -EINVAL;

	if (i >= a->total) {
		void *tmp = realloc(a->array, a->memb * (i + 1));
		if (!tmp)
			return -ENOMEM;

		memset(tmp + (a->memb * a->total), 0, a->memb * (i + 1 - a->total));

		a->array = tmp;
		a->total = i + 1;
	}
	return 0;
}

int
lk_array_set(struct lk_array *a, unsigned int i, const void *e)
{
	int ret = array_resize(a, i);

	if (ret < 0)
		return ret;

	memcpy(a->array + (a->memb * i), e, a->memb);
	a->count++;

	return 0;
}

int
lk_array_unset(struct lk_array *a, unsigned int i)
{
	if (!a || i >= a->total)
		return -EINVAL;

	if (lk_array_exists(a, i)) {
		memset(a->array + (a->memb * i), 0, a->memb);
		a->count--;
	}

	return 0;
}

int
lk_array_append(struct lk_array *a, const void *e)
{
	int ret = array_resize(a, a->count);

	if (ret < 0)
		return ret;

	memcpy(a->array + (a->memb * a->count), e, a->memb);
	a->count++;

	return 0;
}
