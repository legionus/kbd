#include <stdlib.h>
#include <string.h>

#include "nls.h"
#include "kbd.h"

#include "keymap.h"

#include "ksyms.h"
#include "modifiers.h"

int
lk_map_exist(struct keymap *kmap, unsigned int k_table)
{
	return (lk_array_get_ptr(kmap->keymap, k_table) != NULL);
}

int
lk_key_exist(struct keymap *kmap, unsigned int k_table, unsigned int k_index)
{
	struct lk_array *map;
	u_short *key;

	map = lk_array_get_ptr(kmap->keymap, k_table);
	if (!map) {
		return 0;
	}

	key = lk_array_get(map, k_index);
	if (!key) {
		return 0;
	}

	return (*key > 0);
}

int
lk_add_map(struct keymap *kmap, unsigned int k_table)
{
	struct lk_array *keys;

	if (lk_map_exist(kmap, k_table)) {
		return 0;
	}

	keys = malloc(sizeof(struct lk_array));
	if (!keys) {
		ERR(kmap, _("out of memory"));
		return -1;
	}

	lk_array_init(keys, sizeof(unsigned int), 0);

	if (lk_array_set(kmap->keymap, k_table, &keys) < 0) {
		free(keys);
		ERR(kmap, _("out of memory"));
		return -1;
	}

	return 0;
}

int
lk_get_key(struct keymap *kmap, unsigned int k_table, unsigned int k_index)
{
	struct lk_array *map;
	unsigned int *key;

	map = lk_array_get_ptr(kmap->keymap, k_table);
	if (!map) {
		ERR(kmap, _("unable to keymap %d"), k_table);
		return -1;
	}

	key = lk_array_get(map, k_index);
	if (!key || *key == 0) {
		return K_HOLE;
	}

	return (*key)-1;
}

int
lk_del_key(struct keymap *kmap, unsigned int k_table, unsigned int k_index)
{
	struct lk_array *map;

	map = lk_array_get_ptr(kmap->keymap, k_table);
	if (!map) {
		ERR(kmap, _("unable to get keymap %d"), k_table);
		return -1;
	}

	if (!lk_array_exist(map, k_index))
		return 0;

	if (lk_array_unset(map, k_index) < 0) {
		ERR(kmap, _("unable to unset key %d for table %d"),
			k_index, k_table);
		return -1;
	}

	return 0;
}

int
lk_add_key(struct keymap *kmap, unsigned int k_table, unsigned int k_index, int keycode)
{
	struct lk_array *map;
	unsigned int code = keycode + 1;

	if (keycode == CODE_FOR_UNKNOWN_KSYM) {
		/* is safer not to be silent in this case, 
		 * it can be caused by coding errors as well. */
		ERR(kmap, _("lk_add_key called with bad keycode %d"), keycode);
		return -1;
	}

	if (!k_index && keycode == K_NOSUCHMAP)
		return 0;

	map = lk_array_get_ptr(kmap->keymap, k_table);
	if (!map) {
		if (kmap->keywords & LK_KEYWORD_KEYMAPS) {
			ERR(kmap, _("adding map %d violates explicit keymaps line"),
			    k_table);
			return -1;
		}

		if (lk_add_map(kmap, k_table) < 0)
			return -1;
	}

	if ((kmap->keywords & LK_KEYWORD_ALTISMETA) && keycode == K_HOLE &&
	    lk_key_exist(kmap, k_table, k_index))
		return 0;

	map = lk_array_get_ptr(kmap->keymap, k_table);

	if (lk_array_set(map, k_index, &code) < 0) {
		ERR(kmap, _("unable to set key %d for table %d"),
			k_index, k_table);
		return -1;
	}

	if (kmap->keywords & LK_KEYWORD_ALTISMETA) {
		unsigned int alttable = k_table | M_ALT;
		int type = KTYP(keycode);
		int val = KVAL(keycode);

		if (alttable != k_table && !lk_key_exist(kmap, alttable, k_index) &&
		    (type == KT_LATIN || type == KT_LETTER) && val < 128) {
			if (lk_add_map(kmap, alttable) < 0)
				return -1;
			if (lk_add_key(kmap, alttable, k_index, K(KT_META, val)) < 0)
				return -1;
		}
	}

	return 0;
}

int
lk_get_func(struct keymap *kmap, struct kbsentry *kbs)
{
	int x = kbs->kb_func;

	if (x >= MAX_NR_FUNC) {
		ERR(kmap, _("bad index %d"), x);
		return -1;
	}

	if(!(kmap->func_table[x])) {
		ERR(kmap, _("func %d not allocated"), x);
		return -1;
	}

	strncpy((char *)kbs->kb_string, kmap->func_table[x],
		sizeof(kbs->kb_string));
	kbs->kb_string[sizeof(kbs->kb_string) - 1] = 0;

	return 0;
}


int
lk_add_func(struct keymap *kmap, struct kbsentry kbs)
{
	int x;

	x = kbs.kb_func;

	if (x >= MAX_NR_FUNC) {
		ERR(kmap, _("bad func %d"), kbs.kb_func);
		return -1;
	}

	if(kmap->func_table[x]) {
		free(kmap->func_table[x]);
		kmap->func_table[x] = NULL;
	}

	kmap->func_table[x] = strdup((char *)kbs.kb_string);

	if (!kmap->func_table[x]) {
		ERR(kmap, _("out of memory"));
		return -1;
	}

	return 0;
}

int
lk_add_diacr(struct keymap *kmap, unsigned int diacr, unsigned int base, unsigned int res)
{
	accent_entry *ptr;

	if (kmap->accent_table_size == MAX_DIACR) {
		ERR(kmap, _("table overflow"));
		return -1;
	}

	ptr = &(kmap->accent_table[kmap->accent_table_size++]);
	ptr->diacr  = diacr;
	ptr->base   = base;
	ptr->result = res;

	return 0;
}

int
lk_add_compose(struct keymap *kmap,
               unsigned int diacr,
               unsigned int base,
               unsigned int res)
{
	int direction = TO_8BIT;

#ifdef KDSKBDIACRUC
	if (kmap->flags & LK_FLAG_PREFER_UNICODE)
		direction = TO_UNICODE;
#endif
	return lk_add_diacr(kmap,
		convert_code(kmap, diacr, direction),
		convert_code(kmap, base, direction),
		convert_code(kmap, res, direction)
	);
}

static int
do_constant_key(struct keymap *kmap, int i, u_short key)
{
	int typ, val;
	unsigned int j;

	typ = KTYP(key);
	val = KVAL(key);

	if ((typ == KT_LATIN || typ == KT_LETTER) &&
	    ((val >= 'a' && val <= 'z') || (val >= 'A' && val <= 'Z'))) {
		u_short defs[16];
		defs[0] = K(KT_LETTER, val);
		defs[1] = K(KT_LETTER, val ^ 32);
		defs[2] = defs[0];
		defs[3] = defs[1];

		for (j = 4; j < 8; j++)
			defs[j] = K(KT_LATIN, val & ~96);

		for (j = 8; j < 16; j++)
			defs[j] = K(KT_META, KVAL(defs[j - 8]));

		for (j = 0; j < kmap->keymap->total; j++) {
			if (!lk_map_exist(kmap, j))
				continue;

			if (j > 0 && lk_key_exist(kmap, j, i))
				continue;

			if (lk_add_key(kmap, j, i, defs[j % 16]) < 0)
				return -1;
		}

	} else {
		/* do this also for keys like Escape,
		   as promised in the man page */
		for (j = 1; j < kmap->keymap->total; j++) {
			if (!lk_map_exist(kmap, j))
				continue;

			if (lk_key_exist(kmap, j, i))
				continue;

			if (lk_add_key(kmap, j, i, key) < 0)
				return -1;
		}
	}
	return 0;
}

int
lk_add_constants(struct keymap *kmap)
{
	unsigned int i, r0 = 0;

	if (kmap->keywords & LK_KEYWORD_KEYMAPS) {
		while (r0 < kmap->keymap->total && !lk_map_exist(kmap, r0))
			r0++;
	}

	for (i = 0; i < kmap->key_constant->total; i++) {
		char *constant;
		u_short key;

		constant = lk_array_get(kmap->key_constant, i);
		if (!constant || !(*constant))
			continue;

		if (!lk_map_exist(kmap, r0)) {
			ERR(kmap, _("impossible error in lk_add_constants"));
			return -1;
		}

		key = lk_get_key(kmap, r0, i);

		if (do_constant_key(kmap, i, key) < 0)
			return -1;
	}
	return 0;
}
