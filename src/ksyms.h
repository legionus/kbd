#ifndef KSYMS_H
#define KSYMS_H

typedef struct {
	unsigned short uni;
	const char *name;
} sym;

typedef struct {
	const char **table;
	int size;
} syms_entry;

extern syms_entry syms[];

struct syn {
	const char *synonym;
	const char *official_name;
};
extern struct syn synonyms[];

extern const unsigned int syms_size;
extern const unsigned int syn_size;

/* Returned by ksymtocode to report an unknown symbol */
#define CODE_FOR_UNKNOWN_KSYM (-1)

/* Directions for converting keysyms */
#define TO_AUTO (-1)		/* use prefer_unicode */
#define TO_8BIT 0
#define TO_UNICODE 1

extern int set_charset(const char *name);
extern const char *codetoksym(int code);
extern void list_charsets(FILE *f);
extern int ksymtocode(const char *s, int direction);
extern int convert_code(int code, int direction);
extern int add_capslock(int code);

#endif
