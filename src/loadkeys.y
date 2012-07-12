/*
 * loadkeys.y
 *
 * For history, see older versions.
 */

%token EOL NUMBER LITERAL CHARSET KEYMAPS KEYCODE EQUALS
%token PLAIN SHIFT CONTROL ALT ALTGR SHIFTL SHIFTR CTRLL CTRLR CAPSSHIFT
%token COMMA DASH STRING STRLITERAL COMPOSE TO CCHAR ERROR PLUS
%token UNUMBER ALT_IS_META STRINGS AS USUAL ON FOR

%{
#define YY_HEADER_EXPORT_START_CONDITIONS 1

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
#include "nls.h"
#include "version.h"

#include "loadkeys.analyze.h"

#ifdef COMPAT_HEADERS
#include "compat/linux-keyboard.h"
#endif

#define U(x) ((x) ^ 0xf000)

#ifdef KDSKBDIACRUC
typedef struct kbdiacruc accent_entry;
#else
typedef struct kbdiacr accent_entry;
#endif

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

extern int rvalct;
extern struct kbsentry kbs_buf;

char errmsg[1024];

int yyerror(const char *s);
int lkverbose(int level, const char *fmt, ...);
int lkerror(const char *fmt, ...);

extern char *filename;
extern int line_nr;

extern int stack_push(lkfile_t *fp);
extern int prefer_unicode;

#include "ksyms.h"

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
			  "  -C --console=file\n"
			  "                     the console device to be used\n"
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
	if (strlen(errmsg) > 0)
		return(0);

	fprintf(stderr, "%s:%d: %s\n", filename, line_nr, s);
	return (0);
}


int __attribute__ ((format (printf, 2, 3)))
lkverbose(int level, const char *fmt, ...) {
	if (verbose < level)
		return 1;
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stdout, fmt, ap);
	fprintf(stdout, "\n");
	va_end(ap);
	return 0;
}


int __attribute__ ((format (printf, 1, 2)))
lkerror(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fprintf(stdout, "\n");
	va_end(ap);
	return 0;
}


static int
addmap(int i, int explicit)
{
	if (i < 0 || i >= MAX_NR_KEYMAPS) {
		snprintf(errmsg, sizeof(errmsg),
			_("addmap called with bad index %d"), i);
		return -1;
	}

	if (!defining[i]) {
		if (keymaps_line_seen && !explicit) {
			snprintf(errmsg, sizeof(errmsg),
				_("adding map %d violates explicit keymaps line"), i);
			return -1;
		}

		defining[i] = 1;
		if (max_keymap <= i)
			max_keymap = i + 1;
	}
	return 0;
}

/* unset a key */
static int
killkey(int k_index, int k_table)
{
	/* roughly: addkey(k_index, k_table, K_HOLE); */

	if (k_index < 0 || k_index >= NR_KEYS) {
		snprintf(errmsg, sizeof(errmsg),
			_("killkey called with bad index %d"), k_index);
		return -1;
	}

	if (k_table < 0 || k_table >= MAX_NR_KEYMAPS) {
		snprintf(errmsg, sizeof(errmsg),
			_("killkey called with bad table %d"), k_table);
		return -1;
	}

	if (key_map[k_table])
		(key_map[k_table])[k_index] = K_HOLE;

	if (keymap_was_set[k_table])
		(keymap_was_set[k_table])[k_index] = 0;

	return 0;
}

static int
addkey(int k_index, int k_table, int keycode)
{
	int i;

	if (keycode == CODE_FOR_UNKNOWN_KSYM) {
		/* is safer not to be silent in this case, 
		 * it can be caused by coding errors as well. */
		snprintf(errmsg, sizeof(errmsg),
			_("addkey called with bad keycode %d"), keycode);
		return -1;
	}

	if (k_index < 0 || k_index >= NR_KEYS) {
		snprintf(errmsg, sizeof(errmsg),
			_("addkey called with bad index %d"), k_index);
		return -1;
	}

	if (k_table < 0 || k_table >= MAX_NR_KEYMAPS) {
		snprintf(errmsg, sizeof(errmsg),
			_("addkey called with bad table %d"), k_table);
		return -1;
	}

	if (!defining[k_table]) {
		if (addmap(k_table, 0) == -1)
			return -1;
	}

	if (!key_map[k_table]) {
		key_map[k_table] = (u_short *)malloc(NR_KEYS * sizeof(u_short));

		if (key_map[k_table] == NULL) {
			snprintf(errmsg, sizeof(errmsg),
				_("out of memory"));
			return -1;
		}

		for (i = 0; i < NR_KEYS; i++)
			(key_map[k_table])[i] = K_HOLE;
	}

	if (!keymap_was_set[k_table]) {
		keymap_was_set[k_table] = (char *)malloc(NR_KEYS);

		if (key_map[k_table] == NULL) {
			snprintf(errmsg, sizeof(errmsg),
				_("out of memory"));
			return -1;
		}

		for (i = 0; i < NR_KEYS; i++)
			(keymap_was_set[k_table])[i] = 0;
	}

	if (alt_is_meta && keycode == K_HOLE
	    && (keymap_was_set[k_table])[k_index])
		return 0;

	(key_map[k_table])[k_index] = keycode;
	(keymap_was_set[k_table])[k_index] = 1;

	if (alt_is_meta) {
		int alttable = k_table | M_ALT;
		int type = KTYP(keycode);
		int val = KVAL(keycode);

		if (alttable != k_table && defining[alttable] &&
		    (!keymap_was_set[alttable] ||
		     !(keymap_was_set[alttable])[k_index]) &&
		    (type == KT_LATIN || type == KT_LETTER) && val < 128) {
			if (addkey(k_index, alttable, K(KT_META, val)) == -1)
				return -1;
		}
	}
	return 0;
}

static int
addfunc(struct kbsentry kbs)
{
	int sh, i, x;
	char *ptr, *q, *r;

	x = kbs.kb_func;

	if (x >= MAX_NR_FUNC) {
		snprintf(errmsg, sizeof(errmsg),
			_("addfunc called with bad func %d"), kbs.kb_func);
		return -1;
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
		snprintf(errmsg, sizeof(errmsg),
			_("addfunc: func_buf overflow"));
		return -1;
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

	return 0;
}

static int
compose(int diacr, int base, int res)
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
		snprintf(errmsg, sizeof(errmsg),
			_("compose table overflow"));
		return -1;
	}

	ptr = &accent_table[accent_table_size++];
	ptr->diacr = convert_code(diacr, direction);
	ptr->base = convert_code(base, direction);
	ptr->result = convert_code(res, direction);

	return 0;
}

static int defkeys(int fd, int kbd_mode)
{
	struct kbentry ke;
	int ct = 0;
	int i, j, fail;

	if (optu) {
		/* temporarily switch to K_UNICODE while defining keys */
		if (ioctl(fd, KDSKBMODE, K_UNICODE)) {
			lkerror(_("KDSKBMODE: %s: could not switch to Unicode mode"),
				strerror(errno));
			goto fail;
		}
	}

	for (i = 0; i < MAX_NR_KEYMAPS; i++) {
		if (key_map[i]) {
			for (j = 0; j < NR_KEYS; j++) {
				if (!((keymap_was_set[i])[j]))
					continue;

				ke.kb_index = j;
				ke.kb_table = i;
				ke.kb_value = (key_map[i])[j];

				fail = ioctl(fd, KDSKBENT, (unsigned long)&ke);

				if (fail) {
					if (errno == EPERM) {
						lkerror(_("Keymap %d: Permission denied"),
							i);
						j = NR_KEYS;
						continue;
					}
					lkerror("%s", strerror(errno));
				} else
					ct++;

				lkverbose(1, _("keycode %d, table %d = %d%s"),
					j, i, (key_map[i])[j], fail ? _("    FAILED") : "");

				if (fail && !verbose)
					lkerror(_("failed to bind key %d to value %d"),
						j, (key_map[i])[j]);
			}

		} else if (keymaps_line_seen && !defining[i]) {
			/* deallocate keymap */
			ke.kb_index = 0;
			ke.kb_table = i;
			ke.kb_value = K_NOSUCHMAP;

			lkverbose(2, _("deallocate keymap %d"), i);

			if (ioctl(fd, KDSKBENT, (unsigned long)&ke)) {
				if (errno != EINVAL) {
					lkerror(_("KDSKBENT: %s: could not deallocate keymap %d"),
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

						lkerror(_("KDSKBENT: %s: cannot deallocate or clear keymap"),
							strerror(errno));
						goto fail;
					}
				}
			}
		}
	}

	if (optu && ioctl(fd, KDSKBMODE, kbd_mode)) {
		lkerror(_("KDSKBMODE: %s: could not return to original keyboard mode"),
			strerror(errno));
		goto fail;
	}

	return ct;

 fail:	return -1;
}

static void freekeys(void)
{
	int i;
	for (i = 0; i < MAX_NR_KEYMAPS; i++) {
		if (keymap_was_set[i] != NULL)
			free(keymap_was_set[i]);
		if (key_map[i] != NULL)
			free(key_map[i]);
	}
}

static char *
ostr(char *s)
{
	int lth = strlen(s);
	char *ns0 = malloc(4 * lth + 1);
	char *ns = ns0;

	if (ns == NULL) {
		lkerror(_("out of memory"));
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
deffuncs(int fd)
{
	int i, ct = 0;
	char *ptr, *s;
	struct kbsentry kbs;

	for (i = 0; i < MAX_NR_FUNC; i++) {
		kbs.kb_func = i;

		if ((ptr = func_table[i])) {
			strcpy((char *)kbs.kb_string, ptr);
			if (ioctl(fd, KDSKBSENT, (unsigned long)&kbs)) {
				s = ostr((char *)kbs.kb_string);
				if (s == NULL)
					return -1;
				lkerror(_("failed to bind string '%s' to function %s"),
					s, syms[KT_FN].table[kbs.kb_func]);
				free(s);
			} else {
				ct++;
			}
		} else if (opts) {
			kbs.kb_string[0] = 0;

			if (ioctl(fd, KDSKBSENT, (unsigned long)&kbs)) {
				lkerror(_("failed to clear string %s"),
					syms[KT_FN].table[kbs.kb_func]);
			} else {
				ct++;
			}
		}
	}
	return ct;
}

static int
defdiacs(int fd)
{
	unsigned int i, count;
	struct kbdiacrs kd;
#ifdef KDSKBDIACRUC
	struct kbdiacrsuc kdu;
#endif

	count = accent_table_size;
	if (count > MAX_DIACR) {
		count = MAX_DIACR;
		lkerror(_("too many compose definitions"));
	}
#ifdef KDSKBDIACRUC
	if (prefer_unicode) {
		kdu.kb_cnt = count;

		for (i = 0; i < kdu.kb_cnt; i++) {
			kdu.kbdiacruc[i].diacr = accent_table[i].diacr;
			kdu.kbdiacruc[i].base = accent_table[i].base;
			kdu.kbdiacruc[i].result = accent_table[i].result;
		}

		if (ioctl(fd, KDSKBDIACRUC, (unsigned long)&kdu))
			goto fail2;
	} else
#endif
	{
		kd.kb_cnt = count;
		for (i = 0; i < kd.kb_cnt; i++) {
			kd.kbdiacr[i].diacr = accent_table[i].diacr;
			kd.kbdiacr[i].base = accent_table[i].base;
			kd.kbdiacr[i].result = accent_table[i].result;
		}

		if (ioctl(fd, KDSKBDIACR, (unsigned long)&kd))
			goto fail1;
	}

	return kd.kb_cnt;

 fail1:	lkerror("KDSKBDIACR: %s", strerror(errno));
	return -1;

#ifdef KDSKBDIACRUC
 fail2:	lkerror("KDSKBDIACRUC: %s", strerror(errno));
	return -1;
#endif
}

static int
do_constant_key(int i, u_short key)
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

			if (addkey(i, j, defs[j % 16]) == -1)
				return -1;
		}

	} else {
		/* do this also for keys like Escape,
		   as promised in the man page */
		for (j = 1; j < max_keymap; j++) {
			if (defining[j] &&
			    (!(keymap_was_set[j]) || !(keymap_was_set[j])[i])) {
				if (addkey(i, j, key) == -1)
					return -1;
			}
		}
	}
	return 0;
}

static int
do_constant(void)
{
	int i, r0 = 0;

	if (keymaps_line_seen) {
		while (r0 < max_keymap && !defining[r0])
			r0++;
	}

	for (i = 0; i < NR_KEYS; i++) {
		if (key_is_constant[i]) {
			u_short key;

			if (!key_map[r0]) {
				snprintf(errmsg, sizeof(errmsg),
					_("impossible error in do_constant"));
				goto fail;
			}

			key = (key_map[r0])[i];
			if (do_constant_key(i, key) == -1)
				goto fail;
		}
	}
	return 0;

 fail:	lkerror("%s", errmsg);
	return -1;
}

static int
loadkeys(int fd, int kbd_mode)
{
	int keyct, funcct, diacct;

	if ((keyct = defkeys(fd, kbd_mode)) < 0 || (funcct = deffuncs(fd)) < 0)
		return -1;

	lkverbose(1, _("\nChanged %d %s and %d %s"),
		keyct, (keyct == 1) ? _("key") : _("keys"),
		funcct, (funcct == 1) ? _("string") : _("strings"));

	if (accent_table_size > 0 || nocompose) {
		diacct = defdiacs(fd);

		if (diacct < 0)
			return -1;

		lkverbose(1, _("Loaded %d compose %s"),
			diacct, (diacct == 1) ? _("definition") : _("definitions"));

	} else {
		lkverbose(1, _("(No change in compose definitions)"));
	}

	return 0;
}

static int
strings_as_usual(void)
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

			if (addfunc(ke) == -1)
				return -1;
		}
	}
	return 0;
}

static int
compose_as_usual(char *charset)
{
	if (charset && strcmp(charset, "iso-8859-1")) {
		snprintf(errmsg, sizeof(errmsg),
			_("loadkeys: don't know how to compose for %s"),
			charset);
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
			if (compose(ptr.c1, ptr.c2, ptr.c3) == -1)
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

static int
mktable(FILE *fd)
{
	int j;
	unsigned int i, imax;

	char *ptr;
	unsigned int maxfunc;
	unsigned int keymap_count = 0;

	fprintf(fd,
/* not to be translated... */
		    "/* Do not edit this file! It was automatically generated by   */\n");
	fprintf(fd, "/*    loadkeys --mktable defkeymap.map > defkeymap.c          */\n\n");
	fprintf(fd, "#include <linux/types.h>\n");
	fprintf(fd, "#include <linux/keyboard.h>\n");
	fprintf(fd, "#include <linux/kd.h>\n\n");

	for (i = 0; i < MAX_NR_KEYMAPS; i++)
		if (key_map[i]) {
			keymap_count++;
			if (i)
				fprintf(fd, "static ");
			fprintf(fd, "u_short %s_map[NR_KEYS] = {", mk_mapname(i));
			for (j = 0; j < NR_KEYS; j++) {
				if (!(j % 8))
					fprintf(fd, "\n");
				fprintf(fd, "\t0x%04x,", U((key_map[i])[j]));
			}
			fprintf(fd, "\n};\n\n");
		}

	for (imax = MAX_NR_KEYMAPS - 1; imax > 0; imax--)
		if (key_map[imax])
			break;
	fprintf(fd, "ushort *key_maps[MAX_NR_KEYMAPS] = {");
	for (i = 0; i <= imax; i++) {
		fprintf(fd, (i % 4) ? " " : "\n\t");
		if (key_map[i])
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
		if (func_table[maxfunc - 1])
			break;

	fprintf(fd, "char func_buf[] = {\n");
	for (i = 0; i < maxfunc; i++) {
		ptr = func_table[i];
		if (ptr) {
			fprintf(fd, "\t");
			for (; *ptr; ptr++)
				outchar(fd, *ptr, 1);
			fprintf(fd, "0, \n");
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
		if (func_table[i])
			fprintf(fd, "\tfunc_buf + %ld,\n",
			       (long)(func_table[i] - func_buf));
		else
			fprintf(fd, "\t0,\n");
	}
	if (maxfunc < MAX_NR_FUNC)
		fprintf(fd, "\t0,\n");
	fprintf(fd, "};\n");

#ifdef KDSKBDIACRUC
	if (prefer_unicode) {
		fprintf(fd, "\nstruct kbdiacruc accent_table[MAX_DIACR] = {\n");
		for (i = 0; i < accent_table_size; i++) {
			fprintf(fd, "\t{");
			outchar(fd, accent_table[i].diacr, 1);
			outchar(fd, accent_table[i].base, 1);
			fprintf(fd, "0x%04x},", accent_table[i].result);
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
		for (i = 0; i < accent_table_size; i++) {
			fprintf(fd, "\t{");
			outchar(fd, accent_table[i].diacr, 1);
			outchar(fd, accent_table[i].base, 1);
			outchar(fd, accent_table[i].result, 0);
			fprintf(fd, "},");
			if (i % 2)
				fprintf(fd, "\n");
		}
		if (i % 2)
			fprintf(fd, "\n");
		fprintf(fd, "};\n\n");
	}
	fprintf(fd, "unsigned int accent_table_size = %d;\n", accent_table_size);
	return 0;
}

static int
bkeymap(void)
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
	return 0;

 fail:	lkerror(_("Error writing map to file"));
	return -1;
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
				if (set_charset((char *) kbs_buf.kb_string))
					YYERROR;
			}
		;
altismetaline	: ALT_IS_META EOL
			{
				alt_is_meta = 1;
			}
		;
usualstringsline: STRINGS AS USUAL EOL
			{
				if (strings_as_usual() == -1)
					YYERROR;
			}
		;
usualcomposeline: COMPOSE AS USUAL FOR STRLITERAL EOL
			{
				if (compose_as_usual((char *) kbs_buf.kb_string) == -1)
					YYERROR;
			}
		  | COMPOSE AS USUAL EOL
			{
				if (compose_as_usual(0) == -1)
					YYERROR;
			}
		;
keymapline	: KEYMAPS range EOL
			{
				keymaps_line_seen = 1;
			}
		;
range		: range COMMA range0
		| range0
		;
range0		: NUMBER DASH NUMBER
			{
				int i;
				for (i = $1; i <= $3; i++) {
					if (addmap(i,1) == -1)
						YYERROR;
				}
			}
		| NUMBER
			{
				if (addmap($1,1) == -1)
					YYERROR;
			}
		;
strline		: STRING LITERAL EQUALS STRLITERAL EOL
			{
				if (KTYP($2) != KT_FN) {
					snprintf(errmsg, sizeof(errmsg),
						_("'%s' is not a function key symbol"),
						syms[KTYP($2)].table[KVAL($2)]);
					YYERROR;
				}
				kbs_buf.kb_func = KVAL($2);

				if (addfunc(kbs_buf) == -1)
					YYERROR;
			}
		;
compline        : COMPOSE compsym compsym TO compsym EOL
                        {
				if (compose($2, $3, $5) == -1)
					YYERROR;
			}
		 | COMPOSE compsym compsym TO rvalue EOL
			{
				if (compose($2, $3, $5) == -1)
					YYERROR;
			}
                ;
compsym		: CCHAR		{	$$ = $1;		}
		| UNUMBER	{	$$ = $1 ^ 0xf000;	}
		;
singleline	:	{
				mod = 0;
			}
		  modifiers KEYCODE NUMBER EQUALS rvalue EOL
			{
				if (addkey($4, mod, $6) == -1)
					YYERROR;
			}
		| PLAIN KEYCODE NUMBER EQUALS rvalue EOL
			{
				if (addkey($3, 0, $5) == -1)
					YYERROR;
			}
		;
modifiers	: modifiers modifier
		| modifier
		;
modifier	: SHIFT		{ mod |= M_SHIFT;	}
		| CONTROL	{ mod |= M_CTRL;	}
		| ALT		{ mod |= M_ALT;		}
		| ALTGR		{ mod |= M_ALTGR;	}
		| SHIFTL	{ mod |= M_SHIFTL;	}
		| SHIFTR	{ mod |= M_SHIFTR;	}
		| CTRLL		{ mod |= M_CTRLL;	}
		| CTRLR		{ mod |= M_CTRLR;	}
		| CAPSSHIFT	{ mod |= M_CAPSSHIFT;	}
		;
fullline	: KEYCODE NUMBER EQUALS rvalue0 EOL
			{
				int i, j, keycode;

				if (rvalct == 1) {
					/* Some files do not have a keymaps line, and
					 * we have to wait until all input has been read
					 * before we know which maps to fill. */
					key_is_constant[$2] = 1;

					/* On the other hand, we now have include files,
					 * and it should be possible to override lines
					 * from an include file. So, kill old defs. */
					for (j = 0; j < max_keymap; j++) {
						if (!(defining[j]))
							continue;

						if (killkey($2, j) == -1)
							YYERROR;
					}
				}

				if (keymaps_line_seen) {
					i = 0;

					for (j = 0; j < max_keymap; j++) {
						if (!(defining[j]))
							continue;

						if (rvalct != 1 || i == 0) {
							keycode = (i < rvalct)
								? key_buf[i]
								: K_HOLE;

							if (addkey($2, j, keycode) == -1)
								YYERROR;
						}
						i++;
					}

					if (i < rvalct) {
						snprintf(errmsg, sizeof(errmsg),
							_("too many (%d) entries on one line"),
							rvalct);
						YYERROR;
					}
				} else {
					for (i = 0; i < rvalct; i++) {
						if (addkey($2, i, key_buf[i]) == -1)
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
				if (rvalct >= MAX_NR_KEYMAPS) {
					snprintf(errmsg, sizeof(errmsg),
						_("too many key definitions on one line"));
					YYERROR;
				}
				key_buf[rvalct++] = $1;
			}
		;
rvalue		: NUMBER	{ $$ = convert_code($1, TO_AUTO);		}
                | PLUS NUMBER	{ $$ = add_capslock($2);			}
		| UNUMBER	{ $$ = convert_code($1^0xf000, TO_AUTO);	}
		| PLUS UNUMBER	{ $$ = add_capslock($2^0xf000);			}
		| LITERAL	{ $$ = $1;					}
                | PLUS LITERAL	{ $$ = add_capslock($2);			}
		;
%%

static int
parse_keymap(lkfile_t *f)
{
	if (!quiet && !optm)
		lkverbose(1, _("Loading %s"), f->pathname);

	errmsg[0] = '\0';

	if (stack_push(f) == -1) {
		lkerror("%s", errmsg);
		return -1;
	}

	if (yyparse()) {
		if (strlen(errmsg) > 0)
			lkerror("%s", errmsg);
		else
			lkerror(_("syntax error in map file"));

		if (!optm)
			lkerror(_("key bindings not changed"));

		return -1;
	}
	return 0;
}

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
	int c, i, rc = -1;
	int fd;
	int kbd_mode;
	int kd_mode;
	char *console = NULL;
	char *ev;
	lkfile_t f;

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
			prefer_unicode = 1;
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

	/* get console */
	fd = getfd(console);

	if (!optm && !optb) {
		/* check whether the keyboard is in Unicode mode */
		if (ioctl(fd, KDGKBMODE, &kbd_mode) ||
		    ioctl(fd, KDGETMODE, &kd_mode)) {
			fprintf(stderr, _("%s: error reading keyboard mode: %m\n"),
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

		} else if (optu && kd_mode != KD_GRAPHICS) {
			fprintf(stderr,
				_("%s: warning: loading Unicode keymap on non-Unicode console\n"
				  "    (perhaps you want to do `kbd_mode -u'?)\n"),
				progname);
		}
	}

	dirpath = dirpath1;
	if ((ev = getenv("LOADKEYS_KEYMAP_PATH")) != NULL) {
		if (!quiet && !optm)
			fprintf(stdout, _("Searching in %s\n"), ev);

		dirpath2[0] = ev;
		dirpath = dirpath2;
	}

	if (optd) {
		/* first read default map - search starts in . */

		if (findfile(DEFMAP, dirpath, suffixes, &f)) {
			fprintf(stderr, _("Cannot find %s\n"), DEFMAP);
			exit(EXIT_FAILURE);
		}

		if ((rc = parse_keymap(&f)) == -1)
			goto fail;


	} else if (optind == argc) {
		f.fd = stdin;
		strcpy(f.pathname, "<stdin>");

		if ((rc = parse_keymap(&f)) == -1)
			goto fail;
	}

	for (i = optind; argv[i]; i++) {
		if (!strcmp(argv[i], "-")) {
			f.fd = stdin;
			strcpy(f.pathname, "<stdin>");

		} else if (findfile(argv[i], dirpath, suffixes, &f)) {
			fprintf(stderr, _("cannot open file %s\n"), argv[i]);
			goto fail;
		}

		if ((rc = parse_keymap(&f)) == -1)
			goto fail;
	}

	if ((rc = do_constant()) == -1)
		goto fail;

	if (optb) {
		rc = bkeymap();
	} else if (optm) {
		rc = mktable(stdout);
	} else {
		rc = loadkeys(fd, kbd_mode);
	}

 fail:	freekeys();
	close(fd);

	if (rc < 0)
		exit(EXIT_FAILURE);

	exit(EXIT_SUCCESS);
}
