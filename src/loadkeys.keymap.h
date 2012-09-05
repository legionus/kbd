#ifndef LK_KEYMAP_H
#define LK_KEYMAP_H

#include <linux/kd.h>
#include <linux/keyboard.h>
#include "findfile.h"

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

/* LOG_QUIET  - quiet (all messages are disabled)
 * LOG_NORMAL - normal output
 * LOG_VERBOSE{1,2,..} - verbosity levels
 */
typedef enum {
	LOG_QUIET = 0,
	LOG_NORMAL,
	LOG_VERBOSE1,
	LOG_VERBOSE2,
	LOG_VERBOSE3,
	LOG_MAXVALUE
} lkverbosity;

#define MAX_INCLUDE_DEPTH 20

struct keymap {
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

	int prefer_unicode;    

	int rvalct;
	int state_ptr;
	lkfile_t *stack[MAX_INCLUDE_DEPTH];

	/* Verbosity level */
	lkverbosity verbose;

	__attribute__ ((format (printf, 4, 5)))
	void (*log_message)(const char *file, int line, const char *fn, const char *format, ...);

	__attribute__ ((format (printf, 4, 5)))
	void (*log_error)(const char *file, int line, const char *fn, const char *format, ...);
};

#define log_error(kmap, arg...) \
	kmap->log_error(__FILE__, __LINE__, __func__, ## arg)

#define log_verbose(kmap, level, arg...) \
	do { \
		if (kmap->verbose >= level) \
			kmap->log_message(__FILE__, __LINE__, __func__, ## arg); \
	} while(0)

#define MAX_PARSER_STRING 512

struct strdata {
	unsigned int len;
	unsigned char data[MAX_PARSER_STRING];
};

#include <stdio.h>

/* Public */
int keymap_init(struct keymap *km);
void keymap_free(struct keymap *kmap);
int parse_keymap(struct keymap *kmap, lkfile_t *f);

int do_constant(struct keymap *kmap);

int loadkeys(struct keymap *kmap, int fd, int kbd_mode);
int bkeymap(struct keymap *kmap);
int mktable(struct keymap *kmap, FILE *fd);

/* Private */
int stack_push(struct keymap *kmap, lkfile_t *fp, void *scanner);
int stack_pop(struct keymap *kmap, void *scanner);


#endif /* LK_KEYMAP_H */
