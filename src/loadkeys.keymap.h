#ifndef LK_KEYMAP_H
#define LK_KEYMAP_H

#include <linux/kd.h>

#ifdef KDSKBDIACRUC
typedef struct kbdiacruc accent_entry;
#else
typedef struct kbdiacr accent_entry;
#endif

typedef enum {
	LKFLAG_UNICODE_MODE  = (1 << 1),
	LKFLAG_CLEAR_COMPOSE = (1 << 2),
	LKFLAG_CLEAR_STRINGS = (1 << 3),
} lkflags;

struct keymap {
	/* Verbosity level */
	int verbose;

	/* Parser flags */
	lkflags flags;

	/* What keymaps are we defining? */
	char defining[MAX_NR_KEYMAPS];
	int max_keymap;              /* from here on, defining[] is false */

	/* the kernel structures we want to set or print */
	u_short *key_map[MAX_NR_KEYMAPS];
	char *func_table[MAX_NR_FUNC];

	accent_entry accent_table[MAX_DIACR];
	unsigned int accent_table_size;    

	char key_is_constant[NR_KEYS];
	char *keymap_was_set[MAX_NR_KEYMAPS];

	char keymaps_line_seen;      /* Keyword: "keymaps" */
	int alt_is_meta;             /* Keyword: "alt-is-meta" */
	int mod;                     /* Line by line modifiers */
	int key_buf[MAX_NR_KEYMAPS]; /* Key definitions on one line */

	int rvalct;
	unsigned char string[512];

	char errmsg[1024];
	int prefer_unicode;    
};

#endif /* LK_KEYMAP_H */
