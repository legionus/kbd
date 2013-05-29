#ifndef LK_ARRAY_H
#define LK_ARRAY_H

struct lk_array {
	void *array;
	size_t memb;
	size_t count;
	size_t total;
};

int lk_array_init(struct lk_array *a, size_t memb, size_t size);
int lk_array_free(struct lk_array *a);

int lk_array_empty(struct lk_array *a);

int lk_array_append(struct lk_array *a, const void *e);

int   lk_array_set(struct lk_array *a, unsigned int i, const void *e);
void *lk_array_get(struct lk_array *a, unsigned int i);
void *lk_array_get_ptr(struct lk_array *a, unsigned int i);

int lk_array_unset(struct lk_array *a, unsigned int i);
int lk_array_exist(struct lk_array *a, unsigned int i);

#endif /* LK_ARRAY_H */
