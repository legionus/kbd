#include <stdlib.h>
#include <string.h>

#include "nls.h"
#include "kbd.h"

#include "keymap.h"

#include "ksyms.h"
#include "modifiers.h"

int
lk_add_map(struct keymap *kmap, int i, int explicit)
{
	if (i < 0 || i >= MAX_NR_KEYMAPS) {
		ERR(kmap, _("lk_add_map called with bad index %d"), i);
		return -1;
	}

	if (!kmap->defining[i]) {
		if (kmap->keymaps_line_seen && !explicit) {
			ERR(kmap, _("adding map %d violates explicit keymaps line"), i);
			return -1;
		}

		kmap->defining[i] = i+1;
		if (kmap->max_keymap <= i)
			kmap->max_keymap = i + 1;
	}
	return 0;
}

int
lk_get_key(struct keymap *kmap, int k_table, int k_index)
{
	if (k_index < 0 || k_index >= NR_KEYS) {
		ERR(kmap, _("lk_get_key called with bad index %d"), k_index);
		return -1;
	}

	if (k_table < 0 || k_table >= MAX_NR_KEYMAPS) {
		ERR(kmap, _("lk_get_key called with bad table %d"), k_table);
		return -1;
	}

	if (!(kmap->keymap_was_set[k_table]))
		return -1;

	return (kmap->key_map[k_table])[k_index];
}

int
lk_remove_key(struct keymap *kmap, int k_index, int k_table)
{
	/* roughly: addkey(k_index, k_table, K_HOLE); */

	if (k_index < 0 || k_index >= NR_KEYS) {
		ERR(kmap, _("lk_remove_key called with bad index %d"), k_index);
		return -1;
	}

	if (k_table < 0 || k_table >= MAX_NR_KEYMAPS) {
		ERR(kmap, _("lk_remove_key called with bad table %d"), k_table);
		return -1;
	}

	if (kmap->key_map[k_table])
		(kmap->key_map[k_table])[k_index] = K_HOLE;

	if (kmap->keymap_was_set[k_table])
		(kmap->keymap_was_set[k_table])[k_index] = 0;

	return 0;
}

int
lk_add_key(struct keymap *kmap, int k_index, int k_table, int keycode)
{
	int i;

	if (keycode == CODE_FOR_UNKNOWN_KSYM) {
		/* is safer not to be silent in this case, 
		 * it can be caused by coding errors as well. */
		ERR(kmap, _("lk_add_key called with bad keycode %d"), keycode);
		return -1;
	}

	if (k_index < 0 || k_index >= NR_KEYS) {
		ERR(kmap, _("lk_add_key called with bad index %d"), k_index);
		return -1;
	}

	if (k_table < 0 || k_table >= MAX_NR_KEYMAPS) {
		ERR(kmap, _("lk_add_key called with bad table %d"), k_table);
		return -1;
	}

	if (!k_index && keycode == K_NOSUCHMAP)
		return 0;

	if (!kmap->defining[k_table]) {
		if (lk_add_map(kmap, k_table, 0) == -1)
			return -1;
	}

	if (!kmap->key_map[k_table]) {
		kmap->key_map[k_table] = (u_short *)malloc(NR_KEYS * sizeof(u_short));

		if (kmap->key_map[k_table] == NULL) {
			ERR(kmap, _("out of memory"));
			return -1;
		}

		for (i = 0; i < NR_KEYS; i++)
			(kmap->key_map[k_table])[i] = K_HOLE;
	}

	if (!kmap->keymap_was_set[k_table]) {
		kmap->keymap_was_set[k_table] = (char *)malloc(NR_KEYS);

		if (kmap->key_map[k_table] == NULL) {
			ERR(kmap, _("out of memory"));
			return -1;
		}

		for (i = 0; i < NR_KEYS; i++)
			(kmap->keymap_was_set[k_table])[i] = 0;
	}

	if (kmap->alt_is_meta && keycode == K_HOLE
	    && (kmap->keymap_was_set[k_table])[k_index])
		return 0;

	(kmap->key_map[k_table])[k_index] = keycode;
	(kmap->keymap_was_set[k_table])[k_index] = 1;

	if (kmap->alt_is_meta) {
		int alttable = k_table | M_ALT;
		int type = KTYP(keycode);
		int val = KVAL(keycode);

		if (alttable != k_table && kmap->defining[alttable] &&
		    (!kmap->keymap_was_set[alttable] ||
		     !(kmap->keymap_was_set[alttable])[k_index]) &&
		    (type == KT_LATIN || type == KT_LETTER) && val < 128) {
			if (lk_add_key(kmap, k_index, alttable, K(KT_META, val)) == -1)
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
		ERR(kmap, _("lk_add_func called with bad func %d"), kbs.kb_func);
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
		ERR(kmap, _("lk_add_compose table overflow"));
		return -1;
	}

	ptr = &(kmap->accent_table[kmap->accent_table_size++]);
	ptr->diacr  = diacr;
	ptr->base   = base;
	ptr->result = res;

	return 0;
}

int
lk_add_compose(struct keymap *kmap, unsigned int diacr, unsigned int base, unsigned int res)
{
	int direction;

#ifdef KDSKBDIACRUC
	if (kmap->prefer_unicode)
		direction = TO_UNICODE;
	else
#endif
		direction = TO_8BIT;

	return lk_add_diacr(kmap,
		convert_code(kmap, diacr, direction),
		convert_code(kmap, base, direction),
		convert_code(kmap, res, direction)
	);
}

static int
do_constant_key(struct keymap *kmap, int i, u_short key)
{
	int typ, val, j;

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

		for (j = 0; j < kmap->max_keymap; j++) {
			if (!kmap->defining[j])
				continue;

			if (j > 0 &&
			    kmap->keymap_was_set[j] && (kmap->keymap_was_set[j])[i])
				continue;

			if (lk_add_key(kmap, i, j, defs[j % 16]) == -1)
				return -1;
		}

	} else {
		/* do this also for keys like Escape,
		   as promised in the man page */
		for (j = 1; j < kmap->max_keymap; j++) {
			if (kmap->defining[j] &&
			    (!(kmap->keymap_was_set[j]) || !(kmap->keymap_was_set[j])[i])) {
				if (lk_add_key(kmap, i, j, key) == -1)
					return -1;
			}
		}
	}
	return 0;
}

int
lk_add_constants(struct keymap *kmap)
{
	int i, r0 = 0;

	if (kmap->keymaps_line_seen) {
		while (r0 < kmap->max_keymap && !kmap->defining[r0])
			r0++;
	}

	for (i = 0; i < NR_KEYS; i++) {
		if (kmap->key_is_constant[i]) {
			u_short key;

			if (!kmap->key_map[r0]) {
				ERR(kmap, _("impossible error in lk_add_constants"));
				return -1;
			}

			key = lk_get_key(kmap, r0, i);
			if (do_constant_key(kmap, i, key) == -1)
				return -1;
		}
	}
	return 0;
}
