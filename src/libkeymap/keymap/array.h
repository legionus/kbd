#ifndef LK_ARRAY_H
#define LK_ARRAY_H

/**
 * @brief Basic structure for array implementation.
 * @details The array is designed to store an arbitrary number of similar items.
 */
struct lk_array {
	void *array;  /**< Data pointer. */
	size_t memb;  /**< One element size. */
	size_t count; /**< Number of elements. */
	size_t total; /**< Total number of allocated elements. */
};

int lk_array_init(struct lk_array *a, size_t memb, size_t size);
int lk_array_free(struct lk_array *a);

int lk_array_empty(struct lk_array *a);

int lk_array_append(struct lk_array *a, const void *e);

int   lk_array_set(struct lk_array *a, unsigned int i, const void *e);
void *lk_array_get(struct lk_array *a, unsigned int i);
void *lk_array_get_ptr(struct lk_array *a, unsigned int i);

int lk_array_unset(struct lk_array *a, unsigned int i);
int lk_array_exists(struct lk_array *a, unsigned int i);

#endif /* LK_ARRAY_H */
