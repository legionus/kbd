#ifndef LK_DATA_H
#define LK_DATA_H

#include <linux/kd.h>
#include <linux/keyboard.h>
#include <keymap/findfile.h>

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

typedef enum {
	LK_KEYWORD_KEYMAPS    = (1 << 1),
	LK_KEYWORD_ALTISMETA  = (1 << 2),
	LK_KEYWORD_CHARSET    = (1 << 3),
	LK_KEYWORD_STRASUSUAL = (1 << 4)
} lk_keywords;

#define MAX_INCLUDE_DEPTH 20

struct keymap {
	/* Parser flags */
	lkflags flags;

	/* Keymap keywords (keymaps, alt-is-meta, charset, ...) */
	lk_keywords keywords;

	/* What keymaps are we defining? */
	u_short defining[MAX_NR_KEYMAPS];
	int max_keymap;              /* from here on, defining[] is false */

	/* the kernel structures we want to set or print */
	u_short *key_map[MAX_NR_KEYMAPS];
	char *func_table[MAX_NR_FUNC];

	accent_entry accent_table[MAX_DIACR];
	unsigned int accent_table_size;    

	char key_is_constant[NR_KEYS];
	char *keymap_was_set[MAX_NR_KEYMAPS];

	int mod;                     /* Line by line modifiers */
	int key_buf[MAX_NR_KEYMAPS]; /* Key definitions on one line */

	int prefer_unicode;    

	unsigned int charset;

	int rvalct;
	lkfile_t *stack[MAX_INCLUDE_DEPTH];

	int log_priority;
	void (*log_fn)(void *data, int priority,
	               const char *file, int line, const char *fn,
	               const char *format, va_list args);
	void *log_data;
};

#endif /* LK_DATA_H */
