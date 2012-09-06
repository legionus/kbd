/* loadkeys.parse.y
 *
 * This file is part of kbd project.
 * Copyright (C) 1993  Risto Kankkunen.
 * Copyright (C) 1993  Eugene G. Crosser.
 * Copyright (C) 1994-2007  Andries E. Brouwer.
 * Copyright (C) 2007-2012  Alexey Gladkov <gladkov.alexey@gmail.com>
 *
 * This file is covered by the GNU General Public License,
 * which should be included with kbd as the file COPYING.
 */
%{
#define YY_HEADER_EXPORT_START_CONDITIONS 1

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <linux/kd.h>
#include <linux/keyboard.h>
#include <unistd.h>

#include "paths.h"
#include "ksyms.h"
#include "modifiers.h"
#include "nls.h"
#include "kbd.h"

#ifdef COMPAT_HEADERS
#include "compat/linux-keyboard.h"
#endif

#include "loadkeys.parse.h"
#include "loadkeys.analyze.h"
%}

%code requires {
#include "loadkeys.keymap.h"
}

%language "C"
%yacc
%defines
%debug
%error-verbose

/* Pure yylex.  */
%define api.pure
%lex-param { yyscan_t scanner }

/* Pure yyparse.  */
%parse-param { void *scanner }
%parse-param { struct keymap *kmap }

%token EOL NUMBER LITERAL CHARSET KEYMAPS KEYCODE EQUALS
%token PLAIN SHIFT CONTROL ALT ALTGR SHIFTL SHIFTR CTRLL CTRLR CAPSSHIFT
%token COMMA DASH STRING STRLITERAL COMPOSE TO CCHAR ERROR PLUS
%token UNUMBER ALT_IS_META STRINGS AS USUAL ON FOR

%union {
	long long int num;
	struct strdata str;
}

%type <str>  STRLITERAL
%type <num>  CCHAR
%type <num>  LITERAL
%type <num>  NUMBER
%type <num>  UNUMBER

%type <num>  compsym
%type <num>  rvalue

%{

#define U(x) ((x) ^ 0xf000)

int yyerror(yyscan_t scanner, struct keymap *kmap, const char *s);

#include "ksyms.h"

static void attr_fmt45
lkmessage(const char *file attr_unused, int line attr_unused, const char *fn attr_unused,
          const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
#ifdef DEBUG
	fprintf(stdout, "%s(%d): %s: ", file, line, fn);
#endif
	vfprintf(stdout, fmt, ap);
	fprintf(stdout, "\n");
	va_end(ap);
}

static void attr_fmt45
lkerror(const char *file attr_unused, int line attr_unused, const char *fn attr_unused,
        const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	va_end(ap);
}

int
keymap_init(struct keymap *km)
{
	memset(km, 0, sizeof(struct keymap));

	km->verbose     = LOG_NORMAL;
	km->log_message = lkmessage;
	km->log_error   = lkerror;

	return 0;
}

int
yyerror(yyscan_t scanner attr_unused, struct keymap *kmap, const char *s)
{
	log_error(kmap, "%s", s);
	return 0;
}

static int
addmap(struct keymap *kmap, int i, int explicit)
{
	if (i < 0 || i >= MAX_NR_KEYMAPS) {
		log_error(kmap, _("addmap called with bad index %d"), i);
		return -1;
	}

	if (!kmap->defining[i]) {
		if (kmap->keymaps_line_seen && !explicit) {
			log_error(kmap, _("adding map %d violates explicit keymaps line"), i);
			return -1;
		}

		kmap->defining[i] = 1;
		if (kmap->max_keymap <= i)
			kmap->max_keymap = i + 1;
	}
	return 0;
}

/* unset a key */
static int
killkey(struct keymap *kmap, int k_index, int k_table)
{
	/* roughly: addkey(k_index, k_table, K_HOLE); */

	if (k_index < 0 || k_index >= NR_KEYS) {
		log_error(kmap, _("killkey called with bad index %d"), k_index);
		return -1;
	}

	if (k_table < 0 || k_table >= MAX_NR_KEYMAPS) {
		log_error(kmap, _("killkey called with bad table %d"), k_table);
		return -1;
	}

	if (kmap->key_map[k_table])
		(kmap->key_map[k_table])[k_index] = K_HOLE;

	if (kmap->keymap_was_set[k_table])
		(kmap->keymap_was_set[k_table])[k_index] = 0;

	return 0;
}

static int
addkey(struct keymap *kmap, int k_index, int k_table, int keycode)
{
	int i;

	if (keycode == CODE_FOR_UNKNOWN_KSYM) {
		/* is safer not to be silent in this case, 
		 * it can be caused by coding errors as well. */
		log_error(kmap, _("addkey called with bad keycode %d"), keycode);
		return -1;
	}

	if (k_index < 0 || k_index >= NR_KEYS) {
		log_error(kmap, _("addkey called with bad index %d"), k_index);
		return -1;
	}

	if (k_table < 0 || k_table >= MAX_NR_KEYMAPS) {
		log_error(kmap, _("addkey called with bad table %d"), k_table);
		return -1;
	}

	if (!kmap->defining[k_table]) {
		if (addmap(kmap, k_table, 0) == -1)
			return -1;
	}

	if (!kmap->key_map[k_table]) {
		kmap->key_map[k_table] = (u_short *)malloc(NR_KEYS * sizeof(u_short));

		if (kmap->key_map[k_table] == NULL) {
			log_error(kmap, _("out of memory"));
			return -1;
		}

		for (i = 0; i < NR_KEYS; i++)
			(kmap->key_map[k_table])[i] = K_HOLE;
	}

	if (!kmap->keymap_was_set[k_table]) {
		kmap->keymap_was_set[k_table] = (char *)malloc(NR_KEYS);

		if (kmap->key_map[k_table] == NULL) {
			log_error(kmap, _("out of memory"));
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
			if (addkey(kmap, k_index, alttable, K(KT_META, val)) == -1)
				return -1;
		}
	}
	return 0;
}

static int
addfunc(struct keymap *kmap, struct kbsentry kbs)
{
	int x;

	x = kbs.kb_func;

	if (x >= MAX_NR_FUNC) {
		log_error(kmap, _("addfunc called with bad func %d"), kbs.kb_func);
		return -1;
	}

	if(kmap->func_table[x]) {
		free(kmap->func_table[x]);
		kmap->func_table[x] = NULL;
	}

	kmap->func_table[x] = strdup((char *)kbs.kb_string);

	if (!kmap->func_table[x]) {
		log_error(kmap, _("out of memory"));
		return -1;
	}

	return 0;
}

static int
compose(struct keymap *kmap, int diacr, int base, int res)
{
	accent_entry *ptr;
	int direction;

#ifdef KDSKBDIACRUC
	if (kmap->prefer_unicode)
		direction = TO_UNICODE;
	else
#endif
		direction = TO_8BIT;

	if (kmap->accent_table_size == MAX_DIACR) {
		log_error(kmap, _("compose table overflow"));
		return -1;
	}

	ptr = &(kmap->accent_table[kmap->accent_table_size++]);
	ptr->diacr  = convert_code(kmap->prefer_unicode, diacr, direction);
	ptr->base   = convert_code(kmap->prefer_unicode, base, direction);
	ptr->result = convert_code(kmap->prefer_unicode, res, direction);

	return 0;
}

static int
defkeys(struct keymap *kmap, int fd, int kbd_mode)
{
	struct kbentry ke;
	int ct = 0;
	int i, j, fail;

	if (kmap->flags & LKFLAG_UNICODE_MODE) {
		/* temporarily switch to K_UNICODE while defining keys */
		if (ioctl(fd, KDSKBMODE, K_UNICODE)) {
			log_error(kmap, _("KDSKBMODE: %s: could not switch to Unicode mode"),
				strerror(errno));
			goto fail;
		}
	}

	for (i = 0; i < MAX_NR_KEYMAPS; i++) {
		if (kmap->key_map[i]) {
			for (j = 0; j < NR_KEYS; j++) {
				if (!((kmap->keymap_was_set[i])[j]))
					continue;

				ke.kb_index = j;
				ke.kb_table = i;
				ke.kb_value = (kmap->key_map[i])[j];

				fail = ioctl(fd, KDSKBENT, (unsigned long)&ke);

				if (fail) {
					if (errno == EPERM) {
						log_error(kmap, _("Keymap %d: Permission denied"), i);
						j = NR_KEYS;
						continue;
					}
					log_error(kmap, "%s", strerror(errno));
				} else
					ct++;

				log_verbose(kmap, LOG_VERBOSE1, _("keycode %d, table %d = %d%s"),
					j, i, (kmap->key_map[i])[j], fail ? _("    FAILED") : "");

				if (fail && kmap->verbose > 1)
					log_error(kmap, _("failed to bind key %d to value %d"),
						j, (kmap->key_map[i])[j]);
			}

		} else if (kmap->keymaps_line_seen && !kmap->defining[i]) {
			/* deallocate keymap */
			ke.kb_index = 0;
			ke.kb_table = i;
			ke.kb_value = K_NOSUCHMAP;

			log_verbose(kmap, LOG_VERBOSE2, _("deallocate keymap %d"), i);

			if (ioctl(fd, KDSKBENT, (unsigned long)&ke)) {
				if (errno != EINVAL) {
					log_error(kmap, _("KDSKBENT: %s: could not deallocate keymap %d"),
						strerror(errno), i);
					goto fail;
				}
				/* probably an old kernel */
				/* clear keymap by hand */
				for (j = 0; j < NR_KEYS; j++) {
					ke.kb_index = j;
					ke.kb_table = i;
					ke.kb_value = K_HOLE;

					if (ioctl(fd, KDSKBENT, (unsigned long)&ke)) {
						if (errno == EINVAL && i >= 16)
							break;	/* old kernel */

						log_error(kmap, _("KDSKBENT: %s: cannot deallocate or clear keymap"),
							strerror(errno));
						goto fail;
					}
				}
			}
		}
	}

	if ((kmap->flags & LKFLAG_UNICODE_MODE) && ioctl(fd, KDSKBMODE, kbd_mode)) {
		log_error(kmap, _("KDSKBMODE: %s: could not return to original keyboard mode"),
			strerror(errno));
		goto fail;
	}

	return ct;

 fail:	return -1;
}

void
keymap_free(struct keymap *kmap)
{
	int i;
	for (i = 0; i < MAX_NR_KEYMAPS; i++) {
		if (kmap->keymap_was_set[i] != NULL)
			free(kmap->keymap_was_set[i]);
		if (kmap->key_map[i] != NULL)
			free(kmap->key_map[i]);
	}
}

static char *
ostr(struct keymap *kmap, char *s)
{
	int lth = strlen(s);
	char *ns0 = malloc(4 * lth + 1);
	char *ns = ns0;

	if (ns == NULL) {
		log_error(kmap, _("out of memory"));
		return NULL;
	}

	while (*s) {
		switch (*s) {
		case '\n':
			*ns++ = '\\';
			*ns++ = 'n';
			break;
		case '\033':
			*ns++ = '\\';
			*ns++ = '0';
			*ns++ = '3';
			*ns++ = '3';
			break;
		default:
			*ns++ = *s;
		}
		s++;
	}
	*ns = 0;
	return ns0;
}

static int
deffuncs(struct keymap *kmap, int fd)
{
	int i, ct = 0;
	char *ptr, *s;
	struct kbsentry kbs;

	for (i = 0; i < MAX_NR_FUNC; i++) {
		kbs.kb_func = i;

		if ((ptr = kmap->func_table[i])) {
			strcpy((char *)kbs.kb_string, ptr);
			if (ioctl(fd, KDSKBSENT, (unsigned long)&kbs)) {
				s = ostr(kmap, (char *)kbs.kb_string);
				if (s == NULL)
					return -1;
				log_error(kmap, _("failed to bind string '%s' to function %s"),
					s, syms[KT_FN].table[kbs.kb_func]);
				free(s);
			} else {
				ct++;
			}
		} else if (kmap->flags & LKFLAG_CLEAR_STRINGS) {
			kbs.kb_string[0] = 0;

			if (ioctl(fd, KDSKBSENT, (unsigned long)&kbs)) {
				log_error(kmap, _("failed to clear string %s"),
					syms[KT_FN].table[kbs.kb_func]);
			} else {
				ct++;
			}
		}
	}
	return ct;
}

static int
defdiacs(struct keymap *kmap, int fd)
{
	unsigned int i, count;
	struct kbdiacrs kd;
#ifdef KDSKBDIACRUC
	struct kbdiacrsuc kdu;
#endif

	count = kmap->accent_table_size;
	if (count > MAX_DIACR) {
		count = MAX_DIACR;
		log_error(kmap, _("too many compose definitions"));
	}
#ifdef KDSKBDIACRUC
	if (kmap->prefer_unicode) {
		kdu.kb_cnt = count;

		for (i = 0; i < kdu.kb_cnt; i++) {
			kdu.kbdiacruc[i].diacr  = kmap->accent_table[i].diacr;
			kdu.kbdiacruc[i].base   = kmap->accent_table[i].base;
			kdu.kbdiacruc[i].result = kmap->accent_table[i].result;
		}

		if (ioctl(fd, KDSKBDIACRUC, (unsigned long)&kdu))
			goto fail2;
	} else
#endif
	{
		kd.kb_cnt = count;
		for (i = 0; i < kd.kb_cnt; i++) {
			kd.kbdiacr[i].diacr  = kmap->accent_table[i].diacr;
			kd.kbdiacr[i].base   = kmap->accent_table[i].base;
			kd.kbdiacr[i].result = kmap->accent_table[i].result;
		}

		if (ioctl(fd, KDSKBDIACR, (unsigned long)&kd))
			goto fail1;
	}

	return kd.kb_cnt;

 fail1:	log_error(kmap, "KDSKBDIACR: %s", strerror(errno));
	return -1;

#ifdef KDSKBDIACRUC
 fail2:	log_error(kmap, "KDSKBDIACRUC: %s", strerror(errno));
	return -1;
#endif
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

			if (addkey(kmap, i, j, defs[j % 16]) == -1)
				return -1;
		}

	} else {
		/* do this also for keys like Escape,
		   as promised in the man page */
		for (j = 1; j < kmap->max_keymap; j++) {
			if (kmap->defining[j] &&
			    (!(kmap->keymap_was_set[j]) || !(kmap->keymap_was_set[j])[i])) {
				if (addkey(kmap, i, j, key) == -1)
					return -1;
			}
		}
	}
	return 0;
}

static int
do_constant(struct keymap *kmap)
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
				log_error(kmap, _("impossible error in do_constant"));
				return -1;
			}

			key = (kmap->key_map[r0])[i];
			if (do_constant_key(kmap, i, key) == -1)
				return -1;
		}
	}
	return 0;
}


static int
strings_as_usual(struct keymap *kmap)
{
	/*
	 * 26 strings, mostly inspired by the VT100 family
	 */
	char *stringvalues[30] = {
		/* F1 .. F20 */
		"\033[[A",  "\033[[B",  "\033[[C",  "\033[[D",  "\033[[E",
		"\033[17~", "\033[18~", "\033[19~", "\033[20~", "\033[21~",
		"\033[23~", "\033[24~", "\033[25~", "\033[26~",
		"\033[28~", "\033[29~",
		"\033[31~", "\033[32~", "\033[33~", "\033[34~",
		/* Find,    Insert,     Remove,     Select,     Prior */
		"\033[1~",  "\033[2~",  "\033[3~",  "\033[4~",  "\033[5~",
		/* Next,    Macro,      Help,       Do,         Pause */
		"\033[6~",  0,          0,          0,          0
	};
	int i;

	for (i = 0; i < 30; i++) {
		if (stringvalues[i]) {
			struct kbsentry ke;
			ke.kb_func = i;
			strncpy((char *)ke.kb_string, stringvalues[i],
				sizeof(ke.kb_string));
			ke.kb_string[sizeof(ke.kb_string) - 1] = 0;

			if (addfunc(kmap, ke) == -1)
				return -1;
		}
	}
	return 0;
}

static int
compose_as_usual(struct keymap *kmap, char *charset)
{
	if (charset && strcmp(charset, "iso-8859-1")) {
		log_error(kmap, _("loadkeys: don't know how to compose for %s"), charset);
		return -1;

	} else {
		struct ccc {
			unsigned char c1, c2, c3;
		} def_latin1_composes[68] = {
			{ '`', 'A', 0300 }, { '`', 'a', 0340 },
			{ '\'', 'A', 0301 }, { '\'', 'a', 0341 },
			{ '^', 'A', 0302 }, { '^', 'a', 0342 },
			{ '~', 'A', 0303 }, { '~', 'a', 0343 },
			{ '"', 'A', 0304 }, { '"', 'a', 0344 },
			{ 'O', 'A', 0305 }, { 'o', 'a', 0345 },
			{ '0', 'A', 0305 }, { '0', 'a', 0345 },
			{ 'A', 'A', 0305 }, { 'a', 'a', 0345 },
			{ 'A', 'E', 0306 }, { 'a', 'e', 0346 },
			{ ',', 'C', 0307 }, { ',', 'c', 0347 },
			{ '`', 'E', 0310 }, { '`', 'e', 0350 },
			{ '\'', 'E', 0311 }, { '\'', 'e', 0351 },
			{ '^', 'E', 0312 }, { '^', 'e', 0352 },
			{ '"', 'E', 0313 }, { '"', 'e', 0353 },
			{ '`', 'I', 0314 }, { '`', 'i', 0354 },
			{ '\'', 'I', 0315 }, { '\'', 'i', 0355 },
			{ '^', 'I', 0316 }, { '^', 'i', 0356 },
			{ '"', 'I', 0317 }, { '"', 'i', 0357 },
			{ '-', 'D', 0320 }, { '-', 'd', 0360 },
			{ '~', 'N', 0321 }, { '~', 'n', 0361 },
			{ '`', 'O', 0322 }, { '`', 'o', 0362 },
			{ '\'', 'O', 0323 }, { '\'', 'o', 0363 },
			{ '^', 'O', 0324 }, { '^', 'o', 0364 },
			{ '~', 'O', 0325 }, { '~', 'o', 0365 },
			{ '"', 'O', 0326 }, { '"', 'o', 0366 },
			{ '/', 'O', 0330 }, { '/', 'o', 0370 },
			{ '`', 'U', 0331 }, { '`', 'u', 0371 },
			{ '\'', 'U', 0332 }, { '\'', 'u', 0372 },
			{ '^', 'U', 0333 }, { '^', 'u', 0373 },
			{ '"', 'U', 0334 }, { '"', 'u', 0374 },
			{ '\'', 'Y', 0335 }, { '\'', 'y', 0375 },
			{ 'T', 'H', 0336 }, { 't', 'h', 0376 },
			{ 's', 's', 0337 }, { '"', 'y', 0377 },
			{ 's', 'z', 0337 }, { 'i', 'j', 0377 }
		};
		int i;
		for (i = 0; i < 68; i++) {
			struct ccc ptr = def_latin1_composes[i];
			if (compose(kmap, ptr.c1, ptr.c2, ptr.c3) == -1)
				return -1;
		}
	}
	return 0;
}

/*
 * mktable.c
 *
 */
static char *
mk_mapname(char modifier)
{
	static char *modifiers[8] = {
		"shift", "altgr", "ctrl", "alt", "shl", "shr", "ctl", "ctr"
	};
	static char buf[60];
	int i;

	if (!modifier)
		return "plain";
	buf[0] = 0;
	for (i = 0; i < 8; i++)
		if (modifier & (1 << i)) {
			if (buf[0])
				strcat(buf, "_");
			strcat(buf, modifiers[i]);
		}
	return buf;
}

static void
outchar(FILE *fd, unsigned char c, int comma)
{
	fprintf(fd, "'");
	fprintf(fd, (c == '\'' || c == '\\') ? "\\%c" : isgraph(c) ? "%c"
	       : "\\%03o", c);
	fprintf(fd, comma ? "', " : "'");
}

%}

%%
keytable	:
		| keytable line
		;
line		: EOL
		| charsetline
		| altismetaline
		| usualstringsline
		| usualcomposeline
		| keymapline
		| fullline
		| singleline
		| strline
                | compline
		;
charsetline	: CHARSET STRLITERAL EOL
			{
				if (set_charset((char *) $2.data))
					YYERROR;

				/* Unicode: The first 256 code points were made
				   identical to the content of ISO 8859-1 */
				if (kmap->prefer_unicode &&
				    !strcasecmp((char *) $2.data, "iso-8859-1"))
					kmap->prefer_unicode = 0;
			}
		;
altismetaline	: ALT_IS_META EOL
			{
				kmap->alt_is_meta = 1;
			}
		;
usualstringsline: STRINGS AS USUAL EOL
			{
				if (strings_as_usual(kmap) == -1)
					YYERROR;
			}
		;
usualcomposeline: COMPOSE AS USUAL FOR STRLITERAL EOL
			{
				if (compose_as_usual(kmap, (char *) $5.data) == -1)
					YYERROR;
			}
		  | COMPOSE AS USUAL EOL
			{
				if (compose_as_usual(kmap, 0) == -1)
					YYERROR;
			}
		;
keymapline	: KEYMAPS range EOL
			{
				kmap->keymaps_line_seen = 1;
			}
		;
range		: range COMMA range0
		| range0
		;
range0		: NUMBER DASH NUMBER
			{
				int i;
				for (i = $1; i <= $3; i++) {
					if (addmap(kmap, i, 1) == -1)
						YYERROR;
				}
			}
		| NUMBER
			{
				if (addmap(kmap, $1, 1) == -1)
					YYERROR;
			}
		;
strline		: STRING LITERAL EQUALS STRLITERAL EOL
			{
				struct kbsentry ke;

				if (KTYP($2) != KT_FN) {
					log_error(kmap, _("'%s' is not a function key symbol"),
						syms[KTYP($2)].table[KVAL($2)]);
					YYERROR;
				}

				ke.kb_func = KVAL($2);
				strncpy((char *) ke.kb_string,
				        (char *) $4.data,
				        sizeof(ke.kb_string));
				ke.kb_string[sizeof(ke.kb_string) - 1] = 0;

				if (addfunc(kmap, ke) == -1)
					YYERROR;
			}
		;
compline        : COMPOSE compsym compsym TO compsym EOL
                        {
				if (compose(kmap, $2, $3, $5) == -1)
					YYERROR;
			}
		 | COMPOSE compsym compsym TO rvalue EOL
			{
				if (compose(kmap, $2, $3, $5) == -1)
					YYERROR;
			}
                ;
compsym		: CCHAR		{	$$ = $1;		}
		| UNUMBER	{	$$ = $1 ^ 0xf000;	}
		;
singleline	:	{
				kmap->mod = 0;
			}
		  modifiers KEYCODE NUMBER EQUALS rvalue EOL
			{
				if (addkey(kmap, $4, kmap->mod, $6) == -1)
					YYERROR;
			}
		| PLAIN KEYCODE NUMBER EQUALS rvalue EOL
			{
				if (addkey(kmap, $3, 0, $5) == -1)
					YYERROR;
			}
		;
modifiers	: modifiers modifier
		| modifier
		;
modifier	: SHIFT		{ kmap->mod |= M_SHIFT;	}
		| CONTROL	{ kmap->mod |= M_CTRL;	}
		| ALT		{ kmap->mod |= M_ALT;		}
		| ALTGR		{ kmap->mod |= M_ALTGR;	}
		| SHIFTL	{ kmap->mod |= M_SHIFTL;	}
		| SHIFTR	{ kmap->mod |= M_SHIFTR;	}
		| CTRLL		{ kmap->mod |= M_CTRLL;	}
		| CTRLR		{ kmap->mod |= M_CTRLR;	}
		| CAPSSHIFT	{ kmap->mod |= M_CAPSSHIFT;	}
		;
fullline	: KEYCODE NUMBER EQUALS rvalue0 EOL
			{
				int i, j, keycode;

				if (kmap->rvalct == 1) {
					/* Some files do not have a keymaps line, and
					 * we have to wait until all input has been read
					 * before we know which maps to fill. */
					kmap->key_is_constant[$2] = 1;

					/* On the other hand, we now have include files,
					 * and it should be possible to override lines
					 * from an include file. So, kill old defs. */
					for (j = 0; j < kmap->max_keymap; j++) {
						if (!(kmap->defining[j]))
							continue;

						if (killkey(kmap, $2, j) == -1)
							YYERROR;
					}
				}

				if (kmap->keymaps_line_seen) {
					i = 0;

					for (j = 0; j < kmap->max_keymap; j++) {
						if (!(kmap->defining[j]))
							continue;

						if (kmap->rvalct != 1 || i == 0) {
							keycode = (i < kmap->rvalct)
								? kmap->key_buf[i]
								: K_HOLE;

							if (addkey(kmap, $2, j, keycode) == -1)
								YYERROR;
						}
						i++;
					}

					if (i < kmap->rvalct) {
						log_error(kmap, _("too many (%d) entries on one line"),
							kmap->rvalct);
						YYERROR;
					}
				} else {
					for (i = 0; i < kmap->rvalct; i++) {
						if (addkey(kmap, $2, i, kmap->key_buf[i]) == -1)
							YYERROR;
					}
				}
			}
		;

rvalue0		:
		| rvalue1 rvalue0
		;
rvalue1		: rvalue
			{
				if (kmap->rvalct >= MAX_NR_KEYMAPS) {
					log_error(kmap, _("too many key definitions on one line"));
					YYERROR;
				}
				kmap->key_buf[kmap->rvalct++] = $1;
			}
		;
rvalue		: NUMBER	{ $$ = convert_code(kmap->prefer_unicode, $1, TO_AUTO);		}
                | PLUS NUMBER	{ $$ = add_capslock(kmap->prefer_unicode, $2);			}
		| UNUMBER	{ $$ = convert_code(kmap->prefer_unicode, $1^0xf000, TO_AUTO);	}
		| PLUS UNUMBER	{ $$ = add_capslock(kmap->prefer_unicode, $2^0xf000);		}
		| LITERAL	{ $$ = $1;					}
                | PLUS LITERAL	{ $$ = add_capslock(kmap->prefer_unicode, $2);			}
		;
%%

int
parse_keymap(struct keymap *kmap, lkfile_t *f)
{
	yyscan_t scanner;
	int rc = -1;

	yylex_init(&scanner);
	yylex_init_extra(kmap, &scanner);

	log_verbose(kmap, LOG_NORMAL, _("Loading %s"), f->pathname);

	if (stack_push(kmap, f, scanner) == -1)
		goto fail;

	if (yyparse(scanner, kmap))
		goto fail;

	rc = 0;

	stack_pop(kmap, scanner);

 fail:	yylex_destroy(scanner);
	return rc;
}

int
loadkeys(struct keymap *kmap, int fd, int kbd_mode)
{
	int keyct, funcct, diacct;

	if (do_constant(kmap) < 0)
		return -1;

	if ((keyct = defkeys(kmap, fd, kbd_mode)) < 0 || (funcct = deffuncs(kmap, fd)) < 0)
		return -1;

	log_verbose(kmap, LOG_VERBOSE1, _("\nChanged %d %s and %d %s"),
		keyct, (keyct == 1) ? _("key") : _("keys"),
		funcct, (funcct == 1) ? _("string") : _("strings"));

	if (kmap->accent_table_size > 0 || kmap->flags & LKFLAG_CLEAR_COMPOSE) {
		diacct = defdiacs(kmap, fd);

		if (diacct < 0)
			return -1;

		log_verbose(kmap, LOG_VERBOSE1, _("Loaded %d compose %s"),
			diacct, (diacct == 1) ? _("definition") : _("definitions"));

	} else {
		log_verbose(kmap, LOG_VERBOSE1, _("(No change in compose definitions)"));
	}

	return 0;
}

int
bkeymap(struct keymap *kmap)
{
	int i, j;

	//u_char *p;
	char flag, magic[] = "bkeymap";
	unsigned short v;

	if (do_constant(kmap) < 0)
		return -1;

	if (write(1, magic, 7) == -1)
		goto fail;
	for (i = 0; i < MAX_NR_KEYMAPS; i++) {
		flag = kmap->key_map[i] ? 1 : 0;
		if (write(1, &flag, 1) == -1)
			goto fail;
	}
	for (i = 0; i < MAX_NR_KEYMAPS; i++) {
		if (kmap->key_map[i]) {
			for (j = 0; j < NR_KEYS / 2; j++) {
				v = kmap->key_map[i][j];
				if (write(1, &v, 2) == -1)
					goto fail;
			}
		}
	}
	return 0;

 fail:	log_error(kmap, _("Error writing map to file"));
	return -1;
}

int
mktable(struct keymap *kmap, FILE *fd)
{
	int j;
	unsigned int i, imax;

	char *ptr;
	unsigned int maxfunc;
	unsigned int keymap_count = 0;
	unsigned int func_table_offs[MAX_NR_FUNC];
	unsigned int func_buf_offset = 0;

	if (do_constant(kmap) < 0)
		return -1;

	fprintf(fd,
/* not to be translated... */
		    "/* Do not edit this file! It was automatically generated by   */\n");
	fprintf(fd, "/*    loadkeys --mktable defkeymap.map > defkeymap.c          */\n\n");
	fprintf(fd, "#include <linux/types.h>\n");
	fprintf(fd, "#include <linux/keyboard.h>\n");
	fprintf(fd, "#include <linux/kd.h>\n\n");

	for (i = 0; i < MAX_NR_KEYMAPS; i++)
		if (kmap->key_map[i]) {
			keymap_count++;
			if (i)
				fprintf(fd, "static ");
			fprintf(fd, "u_short %s_map[NR_KEYS] = {", mk_mapname(i));
			for (j = 0; j < NR_KEYS; j++) {
				if (!(j % 8))
					fprintf(fd, "\n");
				fprintf(fd, "\t0x%04x,", U((kmap->key_map[i])[j]));
			}
			fprintf(fd, "\n};\n\n");
		}

	for (imax = MAX_NR_KEYMAPS - 1; imax > 0; imax--)
		if (kmap->key_map[imax])
			break;
	fprintf(fd, "ushort *key_maps[MAX_NR_KEYMAPS] = {");
	for (i = 0; i <= imax; i++) {
		fprintf(fd, (i % 4) ? " " : "\n\t");
		if (kmap->key_map[i])
			fprintf(fd, "%s_map,", mk_mapname(i));
		else
			fprintf(fd, "0,");
	}
	if (imax < MAX_NR_KEYMAPS - 1)
		fprintf(fd, "\t0");
	fprintf(fd, "\n};\n\nunsigned int keymap_count = %d;\n\n", keymap_count);

/* uglified just for xgettext - it complains about nonterminated strings */
	fprintf(fd,
	       "/*\n"
	       " * Philosophy: most people do not define more strings, but they who do\n"
	       " * often want quite a lot of string space. So, we statically allocate\n"
	       " * the default and allocate dynamically in chunks of 512 bytes.\n"
	       " */\n" "\n");
	for (maxfunc = MAX_NR_FUNC; maxfunc; maxfunc--)
		if (kmap->func_table[maxfunc - 1])
			break;

	fprintf(fd, "char func_buf[] = {\n");
	for (i = 0; i < maxfunc; i++) {
		ptr = kmap->func_table[i];
		if (ptr) {
			func_table_offs[i] = func_buf_offset;
			fprintf(fd, "\t");
			for (; *ptr; ptr++)
				outchar(fd, *ptr, 1);
			fprintf(fd, "0, \n");
			func_buf_offset += (ptr - kmap->func_table[i] + 1);
		}
	}
	if (!maxfunc)
		fprintf(fd, "\t0\n");
	fprintf(fd, "};\n\n");

	fprintf(fd,
	       "char *funcbufptr = func_buf;\n"
	       "int funcbufsize = sizeof(func_buf);\n"
	       "int funcbufleft = 0;          /* space left */\n" "\n");

	fprintf(fd, "char *func_table[MAX_NR_FUNC] = {\n");
	for (i = 0; i < maxfunc; i++) {
		if (kmap->func_table[i])
			fprintf(fd, "\tfunc_buf + %u,\n", func_table_offs[i]);
		else
			fprintf(fd, "\t0,\n");
	}
	if (maxfunc < MAX_NR_FUNC)
		fprintf(fd, "\t0,\n");
	fprintf(fd, "};\n");

#ifdef KDSKBDIACRUC
	if (kmap->prefer_unicode) {
		fprintf(fd, "\nstruct kbdiacruc accent_table[MAX_DIACR] = {\n");
		for (i = 0; i < kmap->accent_table_size; i++) {
			fprintf(fd, "\t{");
			outchar(fd, kmap->accent_table[i].diacr, 1);
			outchar(fd, kmap->accent_table[i].base, 1);
			fprintf(fd, "0x%04x},", kmap->accent_table[i].result);
			if (i % 2)
				fprintf(fd, "\n");
		}
		if (i % 2)
			fprintf(fd, "\n");
		fprintf(fd, "};\n\n");
	} else
#endif
	{
		fprintf(fd, "\nstruct kbdiacr accent_table[MAX_DIACR] = {\n");
		for (i = 0; i < kmap->accent_table_size; i++) {
			fprintf(fd, "\t{");
			outchar(fd, kmap->accent_table[i].diacr, 1);
			outchar(fd, kmap->accent_table[i].base, 1);
			outchar(fd, kmap->accent_table[i].result, 0);
			fprintf(fd, "},");
			if (i % 2)
				fprintf(fd, "\n");
		}
		if (i % 2)
			fprintf(fd, "\n");
		fprintf(fd, "};\n\n");
	}
	fprintf(fd, "unsigned int accent_table_size = %d;\n", kmap->accent_table_size);
	return 0;
}
