/* A Bison parser, made by GNU Bison 2.4.3.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006,
   2009, 2010 Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.4.3"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
#line 12 "loadkeys.y"

#include <errno.h>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <linux/kd.h>
#include <linux/keyboard.h>
#include <unistd.h>

#include "paths.h"
#include "getfd.h"
#include "findfile.h"
#include "ksyms.h"
#include "modifiers.h"
#include "xmalloc.h"
#include "nls.h"
#include "version.h"

#define U(x) ((x) ^ 0xf000)

#ifdef KDSKBDIACRUC
typedef struct kbdiacruc accent_entry;
#else
typedef struct kbdiacr accent_entry;
#endif

#ifndef KT_LETTER
#define KT_LETTER KT_LATIN
#endif

#undef NR_KEYS
#define NR_KEYS 256

/* What keymaps are we defining? */
char defining[MAX_NR_KEYMAPS];
char keymaps_line_seen = 0;
int max_keymap = 0;	/* from here on, defining[] is false */
int alt_is_meta = 0;

/* the kernel structures we want to set or print */
u_short *key_map[MAX_NR_KEYMAPS];
char *func_table[MAX_NR_FUNC];

accent_entry accent_table[MAX_DIACR];
unsigned int accent_table_size = 0;

char key_is_constant[NR_KEYS];
char *keymap_was_set[MAX_NR_KEYMAPS];
char func_buf[4096];	/* should be allocated dynamically */
char *fp = func_buf;

int key_buf[MAX_NR_KEYMAPS];
int mod;
int private_error_ct = 0;

extern int rvalct;
extern struct kbsentry kbs_buf;

void lkfatal(const char *fmt, ...);
int yyerror(const char *s);

extern char *filename;
extern int line_nr;

extern void stack_push(FILE *fd, int ispipe, char *filename);
extern int prefer_unicode;

#include "ksyms.h"
int yylex(void);

static void attr_noreturn usage(void)
{
	fprintf(stderr, _("loadkeys version %s\n"
			  "\n"
			  "Usage: loadkeys [option...] [mapfile...]\n"
			  "\n"
			  "Valid options are:\n"
			  "\n"
			  "  -a --ascii         force conversion to ASCII\n"
			  "  -b --bkeymap       output a binary keymap to stdout\n"
			  "  -c --clearcompose  clear kernel compose table\n"
			  "  -C <cons1,cons2,...> --console=<cons1,cons2,...>\n"
			  "                     the console device(s) to be used\n"
			  "  -d --default       load \"%s\"\n"
			  "  -h --help          display this help text\n"
			  "  -m --mktable       output a \"defkeymap.c\" to stdout\n"
			  "  -q --quiet         suppress all normal output\n"
			  "  -s --clearstrings  clear kernel string table\n"
			  "  -u --unicode       force conversion to Unicode\n"
			  "  -v --verbose       report the changes\n"),
		PACKAGE_VERSION, DEFMAP);
	exit(EXIT_FAILURE);
}

char **dirpath;
char *dirpath1[] = { "", DATADIR "/" KEYMAPDIR "/**", KERNDIR "/", 0 };
char *dirpath2[] = { 0, 0 };
char *suffixes[] = { "", ".kmap", ".map", 0 };

char **args;
int opta = 0;
int optb = 0;
int optd = 0;
int optm = 0;
int opts = 0;
int optu = 0;
int verbose = 0;
int quiet = 0;
int nocompose = 0;

int yyerror(const char *s)
{
	fprintf(stderr, "%s:%d: %s\n", filename, line_nr, s);
	private_error_ct++;
	return (0);
}

void attr_noreturn attr_format_1_2 lkfatal(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	fprintf(stderr, "%s: %s:%d: ", progname, filename, line_nr);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	va_end(ap);
	exit(EXIT_FAILURE);
}

static void addmap(int i, int explicit)
{
	if (i < 0 || i >= MAX_NR_KEYMAPS)
		lkfatal(_("addmap called with bad index %d"), i);

	if (!defining[i]) {
		if (keymaps_line_seen && !explicit)
			lkfatal(_("adding map %d violates explicit keymaps line"), i);

		defining[i] = 1;
		if (max_keymap <= i)
			max_keymap = i + 1;
	}
}

/* unset a key */
static void killkey(int k_index, int k_table)
{
	/* roughly: addkey(k_index, k_table, K_HOLE); */

	if (k_index < 0 || k_index >= NR_KEYS)
		lkfatal(_("killkey called with bad index %d"), k_index);

	if (k_table < 0 || k_table >= MAX_NR_KEYMAPS)
		lkfatal(_("killkey called with bad table %d"), k_table);

	if (key_map[k_table])
		(key_map[k_table])[k_index] = K_HOLE;

	if (keymap_was_set[k_table])
		(keymap_was_set[k_table])[k_index] = 0;
}

static void addkey(int k_index, int k_table, int keycode)
{
	int i;

	if (keycode == CODE_FOR_UNKNOWN_KSYM)
		/* is safer not to be silent in this case, 
		 * it can be caused by coding errors as well. */
		lkfatal(_("addkey called with bad keycode %d"), keycode);

	if (k_index < 0 || k_index >= NR_KEYS)
		lkfatal(_("addkey called with bad index %d"), k_index);

	if (k_table < 0 || k_table >= MAX_NR_KEYMAPS)
		lkfatal(_("addkey called with bad table %d"), k_table);

	if (!defining[k_table])
		addmap(k_table, 0);

	if (!key_map[k_table]) {
		key_map[k_table] =
		    (u_short *) xmalloc(NR_KEYS * sizeof(u_short));
		for (i = 0; i < NR_KEYS; i++)
			(key_map[k_table])[i] = K_HOLE;
	}

	if (!keymap_was_set[k_table]) {
		keymap_was_set[k_table] = (char *)xmalloc(NR_KEYS);
		for (i = 0; i < NR_KEYS; i++)
			(keymap_was_set[k_table])[i] = 0;
	}

	if (alt_is_meta && keycode == K_HOLE
	    && (keymap_was_set[k_table])[k_index])
		return;

	(key_map[k_table])[k_index] = keycode;
	(keymap_was_set[k_table])[k_index] = 1;

	if (alt_is_meta) {
		int alttable = k_table | M_ALT;
		int type = KTYP(keycode);
		int val = KVAL(keycode);

		if (alttable != k_table && defining[alttable] &&
		    (!keymap_was_set[alttable] ||
		     !(keymap_was_set[alttable])[k_index]) &&
		    (type == KT_LATIN || type == KT_LETTER) && val < 128)
			addkey(k_index, alttable, K(KT_META, val));
	}
}

static void addfunc(struct kbsentry kbs)
{
	int sh, i, x;
	char *ptr, *q, *r;

	x = kbs.kb_func;

	if (x >= MAX_NR_FUNC) {
		fprintf(stderr, _("%s: addfunc called with bad func %d\n"),
			progname, kbs.kb_func);
		exit(EXIT_FAILURE);
	}

	q = func_table[x];
	if (q) {		/* throw out old previous def */
		sh = strlen(q) + 1;
		ptr = q + sh;
		while (ptr < fp)
			*q++ = *ptr++;
		fp -= sh;

		for (i = x + 1; i < MAX_NR_FUNC; i++) {
			if (func_table[i])
				func_table[i] -= sh;
		}
	}

	ptr = func_buf;		/* find place for new def */
	for (i = 0; i < x; i++) {
		if (func_table[i]) {
			ptr = func_table[i];
			while (*ptr++) ;
		}
	}

	func_table[x] = ptr;
	sh = strlen((char *)kbs.kb_string) + 1;

	if (fp + sh > func_buf + sizeof(func_buf)) {
		fprintf(stderr, _("%s: addfunc: func_buf overflow\n"), progname);
		exit(EXIT_FAILURE);
	}
	q = fp;
	fp += sh;
	r = fp;
	while (q > ptr)
		*--r = *--q;
	strcpy(ptr, (char *)kbs.kb_string);
	for (i = x + 1; i < MAX_NR_FUNC; i++) {
		if (func_table[i])
			func_table[i] += sh;
	}
}

static void compose(int diacr, int base, int res)
{
	accent_entry *ptr;
	int direction;

#ifdef KDSKBDIACRUC
	if (prefer_unicode)
		direction = TO_UNICODE;
	else
#endif
		direction = TO_8BIT;

	if (accent_table_size == MAX_DIACR) {
		fprintf(stderr, _("compose table overflow\n"));
		exit(EXIT_FAILURE);
	}

	ptr = &accent_table[accent_table_size++];
	ptr->diacr = convert_code(diacr, direction);
	ptr->base = convert_code(base, direction);
	ptr->result = convert_code(res, direction);
}

static int defkeys(int fd, int kbd_mode)
{
	struct kbentry ke;
	int ct = 0;
	int i, j, fail;

	if (optu) {
		/* temporarily switch to K_UNICODE while defining keys */
		if (ioctl(fd, KDSKBMODE, K_UNICODE)) {
			perror("KDSKBMODE");
			fprintf(stderr, _("%s: could not switch to Unicode mode\n"), progname);
			exit(EXIT_FAILURE);
		}
	}

	for (i = 0; i < MAX_NR_KEYMAPS; i++) {
		if (key_map[i]) {
			for (j = 0; j < NR_KEYS; j++) {
				if ((keymap_was_set[i])[j]) {
					ke.kb_index = j;
					ke.kb_table = i;
					ke.kb_value = (key_map[i])[j];

					fail =
					    ioctl(fd, KDSKBENT,
						  (unsigned long)&ke);
					if (fail) {
						if (errno == EPERM) {
							fprintf(stderr,
								_("Keymap %d: Permission denied\n"),
								i);
							j = NR_KEYS;
							continue;
						}
						perror("KDSKBENT");
					} else
						ct++;
					if (verbose)
						printf(_("keycode %d, table %d = %d%s\n"),
						       j, i, (key_map[i])[j],
						       fail ? _("    FAILED") :
						       "");
					else if (fail)
						fprintf(stderr,
							_("failed to bind key %d to value %d\n"),
							j, (key_map[i])[j]);
				}
			}
			xfree(key_map[i]);
			xfree(keymap_was_set[i]);

		} else if (keymaps_line_seen && !defining[i]) {
			/* deallocate keymap */
			ke.kb_index = 0;
			ke.kb_table = i;
			ke.kb_value = K_NOSUCHMAP;

			if (verbose > 1)
				printf(_("deallocate keymap %d\n"), i);

			if (ioctl(fd, KDSKBENT, (unsigned long)&ke)) {
				if (errno != EINVAL) {
					perror("KDSKBENT");
					fprintf(stderr,
						_("%s: could not deallocate keymap %d\n"),
						progname, i);
					exit(EXIT_FAILURE);
				}
				/* probably an old kernel */
				/* clear keymap by hand */
				for (j = 0; j < NR_KEYS; j++) {
					ke.kb_index = j;
					ke.kb_table = i;
					ke.kb_value = K_HOLE;
					if (ioctl
					    (fd, KDSKBENT,
					     (unsigned long)&ke)) {
						if (errno == EINVAL && i >= 16)
							break;	/* old kernel */
						perror("KDSKBENT");
						fprintf(stderr,
							_("%s: cannot deallocate or clear keymap\n"),
							progname);
						exit(EXIT_FAILURE);
					}
				}
			}
		}
	}

	if (optu && ioctl(fd, KDSKBMODE, kbd_mode)) {
		perror("KDSKBMODE");
		fprintf(stderr,
			_("%s: could not return to original keyboard mode\n"),
			progname);
		exit(EXIT_FAILURE);
	}

	return ct;
}

static char *ostr(char *s)
{
	int lth = strlen(s);
	char *ns0 = xmalloc(4 * lth + 1);
	char *ns = ns0;

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

static int deffuncs(int fd)
{
	int i, ct = 0;
	char *ptr;

	for (i = 0; i < MAX_NR_FUNC; i++) {
		kbs_buf.kb_func = i;

		if ((ptr = func_table[i])) {
			strcpy((char *)kbs_buf.kb_string, ptr);
			if (ioctl(fd, KDSKBSENT, (unsigned long)&kbs_buf))
				fprintf(stderr,
					_("failed to bind string '%s' to function %s\n"),
					ostr((char *)kbs_buf.kb_string),
					syms[KT_FN].table[kbs_buf.kb_func]);
			else
				ct++;
		} else if (opts) {
			kbs_buf.kb_string[0] = 0;
			if (ioctl(fd, KDSKBSENT, (unsigned long)&kbs_buf))
				fprintf(stderr, _("failed to clear string %s\n"),
					syms[KT_FN].table[kbs_buf.kb_func]);
			else
				ct++;
		}
	}
	return ct;
}

static int defdiacs(int fd)
{
	unsigned int i, count;
	struct kbdiacrs kd;
#ifdef KDSKBDIACRUC
	struct kbdiacrsuc kdu;
#endif

	count = accent_table_size;
	if (count > MAX_DIACR) {
		count = MAX_DIACR;
		fprintf(stderr, _("too many compose definitions\n"));
	}
#ifdef KDSKBDIACRUC
	if (prefer_unicode) {
		kdu.kb_cnt = count;

		for (i = 0; i < kdu.kb_cnt; i++) {
			kdu.kbdiacruc[i].diacr = accent_table[i].diacr;
			kdu.kbdiacruc[i].base = accent_table[i].base;
			kdu.kbdiacruc[i].result = accent_table[i].result;
		}

		if (ioctl(fd, KDSKBDIACRUC, (unsigned long)&kdu)) {
			perror("KDSKBDIACRUC");
			exit(EXIT_FAILURE);
		}
	} else
#endif
	{
		kd.kb_cnt = count;
		for (i = 0; i < kd.kb_cnt; i++) {
			kd.kbdiacr[i].diacr = accent_table[i].diacr;
			kd.kbdiacr[i].base = accent_table[i].base;
			kd.kbdiacr[i].result = accent_table[i].result;
		}

		if (ioctl(fd, KDSKBDIACR, (unsigned long)&kd)) {
			perror("KDSKBDIACR");
			exit(EXIT_FAILURE);
		}
	}

	return kd.kb_cnt;
}

static void do_constant_key(int i, u_short key)
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

		for (j = 0; j < max_keymap; j++) {
			if (!defining[j])
				continue;

			if (j > 0 &&
			    keymap_was_set[j] && (keymap_was_set[j])[i])
				continue;

			addkey(i, j, defs[j % 16]);
		}

	} else {
		/* do this also for keys like Escape,
		   as promised in the man page */
		for (j = 1; j < max_keymap; j++) {
			if (defining[j] &&
			    (!(keymap_was_set[j]) || !(keymap_was_set[j])[i]))
				addkey(i, j, key);
		}
	}
}

static void do_constant(void)
{
	int i, r0 = 0;

	if (keymaps_line_seen) {
		while (r0 < max_keymap && !defining[r0])
			r0++;
	}

	for (i = 0; i < NR_KEYS; i++) {
		if (key_is_constant[i]) {
			u_short key;

			if (!key_map[r0])
				lkfatal(_("impossible error in do_constant"));

			key = (key_map[r0])[i];
			do_constant_key(i, key);
		}
	}
}

static void loadkeys(char *console, int kbd_mode)
{
	int fd;
	int keyct, funcct, diacct = 0;

	fd = getfd(console);
	keyct = defkeys(fd, kbd_mode);
	funcct = deffuncs(fd);

	if (verbose) {
		printf(_("\nChanged %d %s and %d %s.\n"),
		       keyct, (keyct == 1) ? _("key") : _("keys"),
		       funcct, (funcct == 1) ? _("string") : _("strings"));
	}

	if (accent_table_size > 0 || nocompose) {
		diacct = defdiacs(fd);

		if (verbose) {
			printf(_("Loaded %d compose %s.\n"),
			       diacct, (diacct == 1) ? _("definition") : _("definitions"));
		}

	} else if (verbose) {
		printf(_("(No change in compose definitions.)\n"));
	}
}

static void strings_as_usual(void)
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
			addfunc(ke);
		}
	}
}

static void compose_as_usual(char *charset)
{
	if (charset && strcmp(charset, "iso-8859-1")) {
		fprintf(stderr, _("loadkeys: don't know how to compose for %s\n"),
			charset);
		exit(EXIT_FAILURE);

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
			compose(ptr.c1, ptr.c2, ptr.c3);
		}
	}
}

/*
 * mktable.c
 *
 */
static char *modifiers[8] = {
	"shift", "altgr", "ctrl", "alt", "shl", "shr", "ctl", "ctr"
};

static char *mk_mapname(char modifier)
{
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

static void outchar(unsigned char c, int comma)
{
	printf("'");
	printf((c == '\'' || c == '\\') ? "\\%c" : isgraph(c) ? "%c"
	       : "\\%03o", c);
	printf(comma ? "', " : "'");
}

static void attr_noreturn mktable(void)
{
	int j;
	unsigned int i, imax;

	char *ptr;
	unsigned int maxfunc;
	unsigned int keymap_count = 0;

	printf(
/* not to be translated... */
		      "/* Do not edit this file! It was automatically generated by   */\n");
	printf
	    ("/*    loadkeys --mktable defkeymap.map > defkeymap.c          */\n\n");
	printf("#include <linux/types.h>\n");
	printf("#include <linux/keyboard.h>\n");
	printf("#include <linux/kd.h>\n\n");

	for (i = 0; i < MAX_NR_KEYMAPS; i++)
		if (key_map[i]) {
			keymap_count++;
			if (i)
				printf("static ");
			printf("u_short %s_map[NR_KEYS] = {", mk_mapname(i));
			for (j = 0; j < NR_KEYS; j++) {
				if (!(j % 8))
					printf("\n");
				printf("\t0x%04x,", U((key_map[i])[j]));
			}
			printf("\n};\n\n");
		}

	for (imax = MAX_NR_KEYMAPS - 1; imax > 0; imax--)
		if (key_map[imax])
			break;
	printf("ushort *key_maps[MAX_NR_KEYMAPS] = {");
	for (i = 0; i <= imax; i++) {
		printf((i % 4) ? " " : "\n\t");
		if (key_map[i])
			printf("%s_map,", mk_mapname(i));
		else
			printf("0,");
	}
	if (imax < MAX_NR_KEYMAPS - 1)
		printf("\t0");
	printf("\n};\n\nunsigned int keymap_count = %d;\n\n", keymap_count);

/* uglified just for xgettext - it complains about nonterminated strings */
	printf("/*\n"
	       " * Philosophy: most people do not define more strings, but they who do\n"
	       " * often want quite a lot of string space. So, we statically allocate\n"
	       " * the default and allocate dynamically in chunks of 512 bytes.\n"
	       " */\n" "\n");
	for (maxfunc = MAX_NR_FUNC; maxfunc; maxfunc--)
		if (func_table[maxfunc - 1])
			break;

	printf("char func_buf[] = {\n");
	for (i = 0; i < maxfunc; i++) {
		ptr = func_table[i];
		if (ptr) {
			printf("\t");
			for (; *ptr; ptr++)
				outchar(*ptr, 1);
			printf("0, \n");
		}
	}
	if (!maxfunc)
		printf("\t0\n");
	printf("};\n\n");

	printf("char *funcbufptr = func_buf;\n"
	       "int funcbufsize = sizeof(func_buf);\n"
	       "int funcbufleft = 0;          /* space left */\n" "\n");

	printf("char *func_table[MAX_NR_FUNC] = {\n");
	for (i = 0; i < maxfunc; i++) {
		if (func_table[i])
			printf("\tfunc_buf + %ld,\n",
			       (long)(func_table[i] - func_buf));
		else
			printf("\t0,\n");
	}
	if (maxfunc < MAX_NR_FUNC)
		printf("\t0,\n");
	printf("};\n");

#ifdef KDSKBDIACRUC
	if (prefer_unicode) {
		printf("\nstruct kbdiacruc accent_table[MAX_DIACR] = {\n");
		for (i = 0; i < accent_table_size; i++) {
			printf("\t{");
			outchar(accent_table[i].diacr, 1);
			outchar(accent_table[i].base, 1);
			printf("0x%04x},", accent_table[i].result);
			if (i % 2)
				printf("\n");
		}
		if (i % 2)
			printf("\n");
		printf("};\n\n");
	} else
#endif
	{
		printf("\nstruct kbdiacr accent_table[MAX_DIACR] = {\n");
		for (i = 0; i < accent_table_size; i++) {
			printf("\t{");
			outchar(accent_table[i].diacr, 1);
			outchar(accent_table[i].base, 1);
			outchar(accent_table[i].result, 0);
			printf("},");
			if (i % 2)
				printf("\n");
		}
		if (i % 2)
			printf("\n");
		printf("};\n\n");
	}
	printf("unsigned int accent_table_size = %d;\n", accent_table_size);

	exit(0);
}

static void attr_noreturn bkeymap(void)
{
	int i, j;

	//u_char *p;
	char flag, magic[] = "bkeymap";
	unsigned short v;

	if (write(1, magic, 7) == -1)
		goto fail;
	for (i = 0; i < MAX_NR_KEYMAPS; i++) {
		flag = key_map[i] ? 1 : 0;
		if (write(1, &flag, 1) == -1)
			goto fail;
	}
	for (i = 0; i < MAX_NR_KEYMAPS; i++) {
		if (key_map[i]) {
			for (j = 0; j < NR_KEYS / 2; j++) {
				v = key_map[i][j];
				if (write(1, &v, 2) == -1)
					goto fail;
			}
		}
	}
	exit(0);

 fail:	fprintf(stderr, _("Error writing map to file\n"));
	exit(1);
}



/* Line 189 of yacc.c  */
#line 937 "loadkeys.c"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     EOL = 258,
     NUMBER = 259,
     LITERAL = 260,
     CHARSET = 261,
     KEYMAPS = 262,
     KEYCODE = 263,
     EQUALS = 264,
     PLAIN = 265,
     SHIFT = 266,
     CONTROL = 267,
     ALT = 268,
     ALTGR = 269,
     SHIFTL = 270,
     SHIFTR = 271,
     CTRLL = 272,
     CTRLR = 273,
     CAPSSHIFT = 274,
     COMMA = 275,
     DASH = 276,
     STRING = 277,
     STRLITERAL = 278,
     COMPOSE = 279,
     TO = 280,
     CCHAR = 281,
     ERROR = 282,
     PLUS = 283,
     UNUMBER = 284,
     ALT_IS_META = 285,
     STRINGS = 286,
     AS = 287,
     USUAL = 288,
     ON = 289,
     FOR = 290
   };
#endif
/* Tokens.  */
#define EOL 258
#define NUMBER 259
#define LITERAL 260
#define CHARSET 261
#define KEYMAPS 262
#define KEYCODE 263
#define EQUALS 264
#define PLAIN 265
#define SHIFT 266
#define CONTROL 267
#define ALT 268
#define ALTGR 269
#define SHIFTL 270
#define SHIFTR 271
#define CTRLL 272
#define CTRLR 273
#define CAPSSHIFT 274
#define COMMA 275
#define DASH 276
#define STRING 277
#define STRLITERAL 278
#define COMPOSE 279
#define TO 280
#define CCHAR 281
#define ERROR 282
#define PLUS 283
#define UNUMBER 284
#define ALT_IS_META 285
#define STRINGS 286
#define AS 287
#define USUAL 288
#define ON 289
#define FOR 290




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef int YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 1049 "loadkeys.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   86

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  36
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  21
/* YYNRULES -- Number of rules.  */
#define YYNRULES  52
/* YYNRULES -- Number of states.  */
#define YYNSTATES  93

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   290

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint8 yyprhs[] =
{
       0,     0,     3,     4,     7,     9,    11,    13,    15,    17,
      19,    21,    23,    25,    27,    31,    34,    39,    46,    51,
      55,    59,    61,    65,    67,    73,    80,    87,    89,    91,
      92,   100,   107,   110,   112,   114,   116,   118,   120,   122,
     124,   126,   128,   130,   136,   137,   140,   142,   144,   147,
     149,   152,   154
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      37,     0,    -1,    -1,    37,    38,    -1,     3,    -1,    39,
      -1,    40,    -1,    41,    -1,    42,    -1,    43,    -1,    53,
      -1,    49,    -1,    46,    -1,    47,    -1,     6,    23,     3,
      -1,    30,     3,    -1,    31,    32,    33,     3,    -1,    24,
      32,    33,    35,    23,     3,    -1,    24,    32,    33,     3,
      -1,     7,    44,     3,    -1,    44,    20,    45,    -1,    45,
      -1,     4,    21,     4,    -1,     4,    -1,    22,     5,     9,
      23,     3,    -1,    24,    48,    48,    25,    48,     3,    -1,
      24,    48,    48,    25,    56,     3,    -1,    26,    -1,    29,
      -1,    -1,    50,    51,     8,     4,     9,    56,     3,    -1,
      10,     8,     4,     9,    56,     3,    -1,    51,    52,    -1,
      52,    -1,    11,    -1,    12,    -1,    13,    -1,    14,    -1,
      15,    -1,    16,    -1,    17,    -1,    18,    -1,    19,    -1,
       8,     4,     9,    54,     3,    -1,    -1,    55,    54,    -1,
      56,    -1,     4,    -1,    28,     4,    -1,    29,    -1,    28,
      29,    -1,     5,    -1,    28,     5,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   877,   877,   878,   880,   881,   882,   883,   884,   885,
     886,   887,   888,   889,   891,   896,   901,   906,   910,   915,
     920,   921,   923,   929,   934,   943,   947,   952,   953,   955,
     955,   962,   967,   968,   970,   971,   972,   973,   974,   975,
     976,   977,   978,   980,  1019,  1020,  1022,  1029,  1030,  1031,
    1032,  1033,  1034
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "EOL", "NUMBER", "LITERAL", "CHARSET",
  "KEYMAPS", "KEYCODE", "EQUALS", "PLAIN", "SHIFT", "CONTROL", "ALT",
  "ALTGR", "SHIFTL", "SHIFTR", "CTRLL", "CTRLR", "CAPSSHIFT", "COMMA",
  "DASH", "STRING", "STRLITERAL", "COMPOSE", "TO", "CCHAR", "ERROR",
  "PLUS", "UNUMBER", "ALT_IS_META", "STRINGS", "AS", "USUAL", "ON", "FOR",
  "$accept", "keytable", "line", "charsetline", "altismetaline",
  "usualstringsline", "usualcomposeline", "keymapline", "range", "range0",
  "strline", "compline", "compsym", "singleline", "$@1", "modifiers",
  "modifier", "fullline", "rvalue0", "rvalue1", "rvalue", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    36,    37,    37,    38,    38,    38,    38,    38,    38,
      38,    38,    38,    38,    39,    40,    41,    42,    42,    43,
      44,    44,    45,    45,    46,    47,    47,    48,    48,    50,
      49,    49,    51,    51,    52,    52,    52,    52,    52,    52,
      52,    52,    52,    53,    54,    54,    55,    56,    56,    56,
      56,    56,    56
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     2,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     3,     2,     4,     6,     4,     3,
       3,     1,     3,     1,     5,     6,     6,     1,     1,     0,
       7,     6,     2,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     5,     0,     2,     1,     1,     2,     1,
       2,     1,     2
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       2,    29,     1,     4,     0,     0,     0,     0,     0,     0,
       0,     0,     3,     5,     6,     7,     8,     9,    12,    13,
      11,     0,    10,     0,    23,     0,    21,     0,     0,     0,
      27,    28,     0,     0,    15,     0,    34,    35,    36,    37,
      38,    39,    40,    41,    42,     0,    33,    14,     0,    19,
       0,    44,     0,     0,     0,     0,     0,     0,    32,    22,
      20,    47,    51,     0,    49,     0,    44,    46,     0,     0,
      18,     0,     0,    16,     0,    48,    52,    50,    43,    45,
       0,    24,     0,    28,     0,     0,     0,    31,    17,    25,
      26,     0,    30
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,     1,    12,    13,    14,    15,    16,    17,    25,    26,
      18,    19,    33,    20,    21,    45,    46,    22,    65,    66,
      67
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -49
static const yytype_int8 yypact[] =
{
     -49,     4,   -49,   -49,   -21,    -1,     9,     7,    16,    37,
       6,   -10,   -49,   -49,   -49,   -49,   -49,   -49,   -49,   -49,
     -49,    43,   -49,    20,    10,     5,   -49,    23,    32,    28,
     -49,   -49,    11,    38,   -49,    35,   -49,   -49,   -49,   -49,
     -49,   -49,   -49,   -49,   -49,    34,   -49,   -49,    61,   -49,
      -1,    12,    62,    47,    -2,    48,    69,    70,   -49,   -49,
     -49,   -49,   -49,    14,   -49,    72,    12,   -49,    12,    73,
     -49,    54,     1,   -49,    71,   -49,   -49,   -49,   -49,   -49,
      75,   -49,    76,   -49,    78,    79,    12,   -49,   -49,   -49,
     -49,    80,   -49
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -49,   -49,   -49,   -49,   -49,   -49,   -49,   -49,   -49,    36,
     -49,   -49,   -33,   -49,   -49,   -49,    39,   -49,    19,   -49,
     -48
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint8 yytable[] =
{
      55,    70,    23,    24,     2,    61,    62,     3,    49,    34,
       4,     5,     6,    27,     7,    28,    61,    62,    75,    76,
      80,    29,    35,    47,    85,    50,     8,    30,     9,    63,
      83,    48,    51,    71,    10,    11,    52,    53,    91,    84,
      63,    64,    57,    77,    54,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    30,    30,    59,    31,    31,    56,    32,
      69,    68,    73,    72,    74,    78,    81,    82,    87,    88,
      86,    89,    90,    92,    58,    79,    60
};

static const yytype_uint8 yycheck[] =
{
      33,     3,    23,     4,     0,     4,     5,     3,     3,     3,
       6,     7,     8,     4,    10,     8,     4,     5,     4,     5,
      68,     5,    32,     3,    72,    20,    22,    26,    24,    28,
      29,    21,     9,    35,    30,    31,     4,     9,    86,    72,
      28,    29,     8,    29,    33,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    26,    26,     4,    29,    29,    33,    32,
      23,     9,     3,    25,     4,     3,     3,    23,     3,     3,
       9,     3,     3,     3,    45,    66,    50
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    37,     0,     3,     6,     7,     8,    10,    22,    24,
      30,    31,    38,    39,    40,    41,    42,    43,    46,    47,
      49,    50,    53,    23,     4,    44,    45,     4,     8,     5,
      26,    29,    32,    48,     3,    32,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    51,    52,     3,    21,     3,
      20,     9,     4,     9,    33,    48,    33,     8,    52,     4,
      45,     4,     5,    28,    29,    54,    55,    56,     9,    23,
       3,    35,    25,     3,     4,     4,     5,    29,     3,    54,
      56,     3,    23,    29,    48,    56,     9,     3,     3,     3,
       3,    56,     3
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  However,
   YYFAIL appears to be in use.  Nevertheless, it is formally deprecated
   in Bison 2.4.2's NEWS entry, where a plan to phase it out is
   discussed.  */

#define YYFAIL		goto yyerrlab
#if defined YYFAIL
  /* This is here to suppress warnings from the GCC cpp's
     -Wunused-macros.  Normally we don't worry about that warning, but
     some users do, and we want to make it easy for users to remove
     YYFAIL uses, which will produce warnings from Bison 2.5.  */
#endif

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}

/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */


/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*-------------------------.
| yyparse or yypush_parse.  |
`-------------------------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{


    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 14:

/* Line 1464 of yacc.c  */
#line 892 "loadkeys.y"
    {
				set_charset((char *) kbs_buf.kb_string);
			}
    break;

  case 15:

/* Line 1464 of yacc.c  */
#line 897 "loadkeys.y"
    {
				alt_is_meta = 1;
			}
    break;

  case 16:

/* Line 1464 of yacc.c  */
#line 902 "loadkeys.y"
    {
				strings_as_usual();
			}
    break;

  case 17:

/* Line 1464 of yacc.c  */
#line 907 "loadkeys.y"
    {
				compose_as_usual((char *) kbs_buf.kb_string);
			}
    break;

  case 18:

/* Line 1464 of yacc.c  */
#line 911 "loadkeys.y"
    {
				compose_as_usual(0);
			}
    break;

  case 19:

/* Line 1464 of yacc.c  */
#line 916 "loadkeys.y"
    {
				keymaps_line_seen = 1;
			}
    break;

  case 22:

/* Line 1464 of yacc.c  */
#line 924 "loadkeys.y"
    {
				int i;
				for (i = (yyvsp[(1) - (3)]); i <= (yyvsp[(3) - (3)]); i++)
					addmap(i,1);
			}
    break;

  case 23:

/* Line 1464 of yacc.c  */
#line 930 "loadkeys.y"
    {
				addmap((yyvsp[(1) - (1)]),1);
			}
    break;

  case 24:

/* Line 1464 of yacc.c  */
#line 935 "loadkeys.y"
    {
				if (KTYP((yyvsp[(2) - (5)])) != KT_FN)
					lkfatal(_("'%s' is not a function key symbol"),
						syms[KTYP((yyvsp[(2) - (5)]))].table[KVAL((yyvsp[(2) - (5)]))]);
				kbs_buf.kb_func = KVAL((yyvsp[(2) - (5)]));
				addfunc(kbs_buf);
			}
    break;

  case 25:

/* Line 1464 of yacc.c  */
#line 944 "loadkeys.y"
    {
				compose((yyvsp[(2) - (6)]), (yyvsp[(3) - (6)]), (yyvsp[(5) - (6)]));
			}
    break;

  case 26:

/* Line 1464 of yacc.c  */
#line 948 "loadkeys.y"
    {
				compose((yyvsp[(2) - (6)]), (yyvsp[(3) - (6)]), (yyvsp[(5) - (6)]));
			}
    break;

  case 27:

/* Line 1464 of yacc.c  */
#line 952 "loadkeys.y"
    {	(yyval) = (yyvsp[(1) - (1)]);		}
    break;

  case 28:

/* Line 1464 of yacc.c  */
#line 953 "loadkeys.y"
    {	(yyval) = (yyvsp[(1) - (1)]) ^ 0xf000;	}
    break;

  case 29:

/* Line 1464 of yacc.c  */
#line 955 "loadkeys.y"
    {
				mod = 0;
			}
    break;

  case 30:

/* Line 1464 of yacc.c  */
#line 959 "loadkeys.y"
    {
				addkey((yyvsp[(4) - (7)]), mod, (yyvsp[(6) - (7)]));
			}
    break;

  case 31:

/* Line 1464 of yacc.c  */
#line 963 "loadkeys.y"
    {
				addkey((yyvsp[(3) - (6)]), 0, (yyvsp[(5) - (6)]));
			}
    break;

  case 34:

/* Line 1464 of yacc.c  */
#line 970 "loadkeys.y"
    { mod |= M_SHIFT;	}
    break;

  case 35:

/* Line 1464 of yacc.c  */
#line 971 "loadkeys.y"
    { mod |= M_CTRL;	}
    break;

  case 36:

/* Line 1464 of yacc.c  */
#line 972 "loadkeys.y"
    { mod |= M_ALT;		}
    break;

  case 37:

/* Line 1464 of yacc.c  */
#line 973 "loadkeys.y"
    { mod |= M_ALTGR;	}
    break;

  case 38:

/* Line 1464 of yacc.c  */
#line 974 "loadkeys.y"
    { mod |= M_SHIFTL;	}
    break;

  case 39:

/* Line 1464 of yacc.c  */
#line 975 "loadkeys.y"
    { mod |= M_SHIFTR;	}
    break;

  case 40:

/* Line 1464 of yacc.c  */
#line 976 "loadkeys.y"
    { mod |= M_CTRLL;	}
    break;

  case 41:

/* Line 1464 of yacc.c  */
#line 977 "loadkeys.y"
    { mod |= M_CTRLR;	}
    break;

  case 42:

/* Line 1464 of yacc.c  */
#line 978 "loadkeys.y"
    { mod |= M_CAPSSHIFT;	}
    break;

  case 43:

/* Line 1464 of yacc.c  */
#line 981 "loadkeys.y"
    {
				int i, j;

				if (rvalct == 1) {
					/* Some files do not have a keymaps line, and
					 * we have to wait until all input has been read
					 * before we know which maps to fill. */
					key_is_constant[(yyvsp[(2) - (5)])] = 1;

					/* On the other hand, we now have include files,
					 * and it should be possible to override lines
					 * from an include file. So, kill old defs. */
					for (j = 0; j < max_keymap; j++) {
						if (defining[j])
							killkey((yyvsp[(2) - (5)]), j);
					}
				}

				if (keymaps_line_seen) {
					i = 0;

					for (j = 0; j < max_keymap; j++) {
						if (defining[j]) {
							if (rvalct != 1 || i == 0)
								addkey((yyvsp[(2) - (5)]), j, (i < rvalct) ? key_buf[i] : K_HOLE);
							i++;
						}
					}

					if (i < rvalct)
						lkfatal(_("too many (%d) entries on one line"), rvalct);
				} else {
					for (i = 0; i < rvalct; i++)
						addkey((yyvsp[(2) - (5)]), i, key_buf[i]);
				}
			}
    break;

  case 46:

/* Line 1464 of yacc.c  */
#line 1023 "loadkeys.y"
    {
				if (rvalct >= MAX_NR_KEYMAPS)
					lkfatal(_("too many key definitions on one line"));
				key_buf[rvalct++] = (yyvsp[(1) - (1)]);
			}
    break;

  case 47:

/* Line 1464 of yacc.c  */
#line 1029 "loadkeys.y"
    { (yyval) = convert_code((yyvsp[(1) - (1)]), TO_AUTO);		}
    break;

  case 48:

/* Line 1464 of yacc.c  */
#line 1030 "loadkeys.y"
    { (yyval) = add_capslock((yyvsp[(2) - (2)]));			}
    break;

  case 49:

/* Line 1464 of yacc.c  */
#line 1031 "loadkeys.y"
    { (yyval) = convert_code((yyvsp[(1) - (1)])^0xf000, TO_AUTO);	}
    break;

  case 50:

/* Line 1464 of yacc.c  */
#line 1032 "loadkeys.y"
    { (yyval) = add_capslock((yyvsp[(2) - (2)])^0xf000);			}
    break;

  case 51:

/* Line 1464 of yacc.c  */
#line 1033 "loadkeys.y"
    { (yyval) = (yyvsp[(1) - (1)]);					}
    break;

  case 52:

/* Line 1464 of yacc.c  */
#line 1034 "loadkeys.y"
    { (yyval) = add_capslock((yyvsp[(2) - (2)]));			}
    break;



/* Line 1464 of yacc.c  */
#line 2635 "loadkeys.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



/* Line 1684 of yacc.c  */
#line 1036 "loadkeys.y"


int main(int argc, char *argv[])
{
	const char *short_opts = "abcC:dhmsuqvV";
	const struct option long_opts[] = {
		{ "console", required_argument, NULL, 'C'},
		{ "ascii",		no_argument, NULL, 'a' },
		{ "bkeymap",		no_argument, NULL, 'b' },
		{ "clearcompose",	no_argument, NULL, 'c' },
		{ "default",		no_argument, NULL, 'd' },
		{ "help",		no_argument, NULL, 'h' },
		{ "mktable",		no_argument, NULL, 'm' },
		{ "clearstrings",	no_argument, NULL, 's' },
		{ "unicode",		no_argument, NULL, 'u' },
		{ "quiet",		no_argument, NULL, 'q' },
		{ "verbose",		no_argument, NULL, 'v' },
		{ "version",		no_argument, NULL, 'V' },
		{ NULL, 0, NULL, 0 }
	};
	int c, i;
	int fd;
	int kbd_mode;
	int kd_mode;
	char *console = NULL;

	set_progname(argv[0]);

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE_NAME, LOCALEDIR);
	textdomain(PACKAGE_NAME);

	while ((c = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
		switch (c) {
		case 'a':
			opta = 1;
			break;
		case 'b':
			optb = 1;
			break;
		case 'c':
			nocompose = 1;
			break;
		case 'C':
			console = optarg;
			break;
		case 'd':
			optd = 1;
			break;
		case 'm':
			optm = 1;
			break;
		case 's':
			opts = 1;
			break;
		case 'u':
			optu = 1;
			break;
		case 'q':
			quiet = 1;
			break;
		case 'v':
			verbose++;
			break;
		case 'V':
			print_version_and_exit();
		case 'h':
		case '?':
			usage();
		}
	}

	if (optu && opta) {
		fprintf(stderr,
			_("%s: Options --unicode and --ascii are mutually exclusive\n"),
			progname);
		exit(EXIT_FAILURE);
	}

	prefer_unicode = optu;

	if (!optm && !optb) {
		/* check whether the keyboard is in Unicode mode */
		fd = getfd(console);

		if (ioctl(fd, KDGKBMODE, &kbd_mode)) {
			perror("KDGKBMODE");
			fprintf(stderr, _("%s: error reading keyboard mode\n"),
				progname);
			exit(EXIT_FAILURE);
		}

		if (kbd_mode == K_UNICODE) {
			if (opta) {
				fprintf(stderr,
					_("%s: warning: loading non-Unicode keymap on Unicode console\n"
					  "    (perhaps you want to do `kbd_mode -a'?)\n"),
					progname);
			} else {
				prefer_unicode = 1;
			}

			/* reset -u option if keyboard is in K_UNICODE anyway */
			optu = 0;
		} else if (optu
			   && (ioctl(fd, KDGETMODE, &kd_mode) || (kd_mode != KD_GRAPHICS)))
			fprintf(stderr,
				_("%s: warning: loading Unicode keymap on non-Unicode console\n"
				  "    (perhaps you want to do `kbd_mode -u'?)\n"),
				progname);

		close(fd);
	}

	for (i = optind; argv[i]; i++) {
		FILE *f;
		char *ev;

		dirpath = dirpath1;
		if ((ev = getenv("LOADKEYS_KEYMAP_PATH")) != NULL) {
			if (!quiet && !optm)
				fprintf(stdout, _("Searching in %s\n"), ev);

			dirpath2[0] = ev;
			dirpath = dirpath2;
		}

		if (optd) {
			/* first read default map - search starts in . */
			optd = 0;
			if ((f = findfile(DEFMAP, dirpath, suffixes)) == NULL) {
				fprintf(stderr, _("Cannot find %s\n"), DEFMAP);
				exit(EXIT_FAILURE);
			}
			goto gotf;
		}

		if (!strcmp(argv[i], "-")) {
			f = stdin;
			strcpy(pathname, "<stdin>");

		} else if ((f = findfile(argv[i], dirpath, suffixes)) == NULL) {
			fprintf(stderr, _("cannot open file %s\n"), argv[i]);
			exit(EXIT_FAILURE);
		}

 gotf:
		if (!quiet && !optm)
			fprintf(stdout, _("Loading %s\n"), pathname);

		stack_push(f, 0, pathname);

		if (yyparse()) {
			fprintf(stderr, _("syntax error in map file\n"));

			if (!optm)
				fprintf(stderr,
					_("key bindings not changed\n"));
			exit(EXIT_FAILURE);
		}
	}

	do_constant();

	if (optb) {
		bkeymap();

	} else if (optm) {
		mktable();

	} else if (console) {
		char *buf = strdup(console);	/* make writable */
		char *e, *s = buf;
		while (*s) {
			char ch;

			while (*s == ' ' || *s == '\t' || *s == ',')
				s++;
			e = s;
			while (*e && *e != ' ' && *e != '\t' && *e != ',')
				e++;
			ch = *e;
			*e = '\0';

			if (verbose)
				printf("%s\n", s);

			loadkeys(s, kbd_mode);

			*e = ch;
			s = e;
		}
		free(buf);

	} else {
		loadkeys(NULL, kbd_mode);
	}
	exit(EXIT_SUCCESS);
}

