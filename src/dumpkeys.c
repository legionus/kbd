/*
 * dumpkeys.c
 *
 * derived from version 0.81 - aeb@cwi.nl
 */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <getopt.h>
#include <linux/types.h>
#include <linux/kd.h>
#include <linux/keyboard.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include "ksyms.h"
#include "getfd.h"
#include "modifiers.h"
#include "nls.h"
#include "version.h"

#ifndef KT_LETTER
#define KT_LETTER KT_LATIN
#endif

#ifndef MAX_NR_KEYMAPS
#define MAX_NR_KEYMAPS NR_KEYMAPS
#endif

static int fd;
static int verbose;
static int nr_keys = 0;			/* probably 112, 128, 256 or 512 */

int keymap_index[MAX_NR_KEYMAPS];	/* inverse of good_keymap */
int good_keymap[MAX_NR_KEYMAPS], keymapnr, allocct;

/* note: asking for n > 255 is not meaningful: ke.kb_index is uchar */
static int
has_key(int n) {
	struct kbentry ke;

	ke.kb_table = 0;	/* plain map is always present */
	ke.kb_index = n;
	return !ioctl(fd, KDGKBENT, (unsigned long)&ke);
}

static void
find_nr_keys(void) {
	nr_keys = (has_key(255) ? 256 : has_key(127) ? 128 : 112);
}

static void
get_keymaps(void) {
	int i, j;
	struct kbentry ke;

	keymapnr = allocct = 0;
	for (i=0; i<MAX_NR_KEYMAPS; i++) {
	    ke.kb_index = 0;
	    ke.kb_table = i;
	    j = ioctl(fd, KDGKBENT, (unsigned long)&ke);
	    if (j && errno != EINVAL) {
		perror("KDGKBENT");
		fprintf(stderr,
			_("KDGKBENT error at index 0 in table %d\n"), i);
		exit(1);
	    }
	    if (!j && ke.kb_value != K_NOSUCHMAP) {
		keymap_index[i] = keymapnr;
		good_keymap[keymapnr++] = i;
		if (ke.kb_value == K_ALLOCATED)
		  allocct++;
	    } else {
		keymap_index[i] = -1;
	    }
	}
	if (keymapnr == 0) {
	    fprintf(stderr, _("%s: cannot find any keymaps?\n"), progname);
	    exit(1);
	}
	if (good_keymap[0] != 0) {
	    fprintf(stderr,
		    _("%s: plain map not allocated? very strange ...\n"),
		    progname);
	    /* this is not fatal */
	}
}

static void
print_keymaps(void) {
	int i,m0,m;

	printf("keymaps ");
	for (i=0; i<keymapnr; i++) {
	    if (i)
	      printf(",");
	    m0 = m = good_keymap[i];
	    while (i+1 < keymapnr && good_keymap[i+1] == m+1)
	      i++, m++;
	    if (m0 == m)
	      printf("%d", m0);
	    else
	      printf("%d-%d", m0, m);
	}
	printf("\n");
}

static int
get_bind(u_char kb_index, u_char kb_table) {
	struct kbentry ke;

	ke.kb_index = kb_index;
	ke.kb_table = kb_table;
	if (ioctl(fd, KDGKBENT, (unsigned long)&ke)) {
		if (kb_index < 128) {
			perror("KDGKBENT");
			fprintf(stderr, _("KDGKBENT error at index %d in table %d\n"),
				kb_index, kb_table);
			exit(1);
		} else
			return -1;
	}
	return ke.kb_value;
}

static void
print_keysym(int code, char numeric) {
	unsigned int t;
	int v;
	const char *p;
	int plus;

	printf(" ");
	t = KTYP(code);
	v = KVAL(code);
	if (t >= syms_size) {
		if (!numeric && (p = codetoksym(code)) != NULL)
			printf("%-16s", p);
		else
			printf("U+%04x          ", code ^ 0xf000);
		return;
	}
	plus = 0;
	if (t == KT_LETTER) {
		t = KT_LATIN;
		printf("+");
		plus++;
	}
	if (!numeric && t < syms_size && v < syms[t].size &&
	    (p = syms[t].table[v])[0])
		printf("%-*s", 16 - plus, p);
	else if (!numeric && t == KT_META && v < 128 && v < syms[0].size &&
		 (p = syms[0].table[v])[0])
		printf("Meta_%-11s", p);
	else
		printf("0x%04x         %s", code, plus ? "" : " ");
}

static char
valid_type(int t) {
	struct kbentry ke;
	char status;

	ke.kb_index = 0;
	ke.kb_table = 0;
	ke.kb_value = K(t, 0);
	status = (ioctl(fd, KDSKBENT, (unsigned long)&ke) == 0);
	return status;
}

static u_char
maximum_val(int t) {
	struct kbentry ke, ke0;
	int i;

	ke.kb_index = 0;
	ke.kb_table = 0;
	ke.kb_value = K_HOLE;
	ke0 = ke;
	ioctl(fd, KDGKBENT, (unsigned long)&ke0);

	for (i = 0; i < 256; i++) {
		ke.kb_value = K(t, i);
		if (ioctl(fd, KDSKBENT, (unsigned long)&ke))
			break;
	}
	ke.kb_value = K_HOLE;
	ioctl(fd, KDSKBENT, (unsigned long)&ke0);

	return i - 1;
}

#define NR_TYPES 15
int maxval[NR_TYPES];

#ifdef KDGKBDIACR
/* isgraph() does not know about iso-8859; printing the character
   unescaped makes the output easier to check. Maybe this should
   be an option. Use locale? */
static void
outchar (unsigned char c) {
	printf("'");
	printf((c == '\'' || c == '\\') ? "\\%c"
	       : (isgraph(c) || c == ' ' || c >= 0200) ? "%c"
	       : "\\%03o", c);
	printf("'");
}

#ifdef KDGKBDIACRUC
static struct kbdiacrsuc kd;
#else
static struct kbdiacrs kd;
#endif

static void
get_diacs(void) {
	static int got_diacs = 0;

#ifdef KDGKBDIACRUC
	if(!got_diacs && ioctl(fd, KDGKBDIACRUC, (unsigned long)&kd)) {
	    perror("KDGKBDIACRUC");
	    exit(1);
	}
#else
	if(!got_diacs && ioctl(fd, KDGKBDIACR, (unsigned long)&kd)) {
	    perror("KDGKBDIACR");
	    exit(1);
	}
#endif

	got_diacs = 1;
}

static int
nr_of_diacs(void) {
	get_diacs();
	return kd.kb_cnt;
}

static void
dump_diacs(void) {
	unsigned int i;

	get_diacs();
#ifdef KDGKBDIACRUC
	for (i = 0; i < kd.kb_cnt; i++) {
		printf("compose ");
		outchar(kd.kbdiacruc[i].diacr & 0xff);
		printf(" ");
		outchar(kd.kbdiacruc[i].base & 0xff);
		printf(" to U+%04x\n", kd.kbdiacruc[i].result);
	}
#else
	for (i = 0; i < kd.kb_cnt; i++) {
		printf("compose ");
		outchar(kd.kbdiacr[i].diacr);
		printf(" ");
		outchar(kd.kbdiacr[i].base);
		printf(" to ");
		outchar(kd.kbdiacr[i].result);
		printf("\n");
	}
#endif
}
#endif

static void
show_short_info(void) {
	int i;

	printf(_("keycode range supported by kernel:           1 - %d\n"),
	       nr_keys - 1);
	printf(_("max number of actions bindable to a key:         %d\n"),
	       MAX_NR_KEYMAPS);
	get_keymaps();
	printf(_("number of keymaps in actual use:                 %d\n"),
	       keymapnr);
	if (allocct)
	  printf(_("of which %d dynamically allocated\n"), allocct);
	printf(_("ranges of action codes supported by kernel:\n"));
	for (i = 0; i < NR_TYPES && valid_type(i); i++) {
	    maxval[i] = maximum_val(i);
	    printf("	0x%04x - 0x%04x\n", K(i, 0), K(i, maxval[i]));
	}
	printf(_("number of function keys supported by kernel: %d\n"),
	       MAX_NR_FUNC);

	printf(_("max nr of compose definitions: %d\n"),
	       MAX_DIACR);
	printf(_("nr of compose definitions in actual use: %d\n"),
	       nr_of_diacs());
}

static struct {
    char *name;
    int bit;
} modifiers[] = {
    { "shift",	KG_SHIFT  },
    { "altgr",	KG_ALTGR  },
    { "control",KG_CTRL   },
    { "alt",	KG_ALT    },
    { "shiftl",	KG_SHIFTL },
    { "shiftr",	KG_SHIFTR },
    { "ctrll",	KG_CTRLL  },
    { "ctrlr",	KG_CTRLR  },
    { "capsshift",	KG_CAPSSHIFT  }
};

static void
dump_symbols(void) {
	unsigned int t;
	int v;
	const char *p;

	printf(_("Symbols recognized by %s:\n(numeric value, symbol)\n\n"),
	       progname);
	for (t = 0; t < syms_size; t++) {
	    if (syms[t].size) {
		for (v = 0; v < syms[t].size; v++)
			if ((p = syms[t].table[v])[0])
				printf("0x%04x\t%s\n", K(t, v), p);
	    } else if (t == KT_META) {
		for (v = 0; v < syms[0].size && v < 128; v++)
			if ((p = syms[0].table[v])[0])
				printf("0x%04x\tMeta_%s\n", K(t, v), p);
	    }
	}
	printf(_("\nThe following synonyms are recognized:\n\n"));
	for (t = 0; t < syn_size; t++)
	  printf(_("%-15s for %s\n"), synonyms[t].synonym,
		 synonyms[t].official_name);
	printf(_("\nRecognized modifier names and their column numbers:\n"));
	for (t = 0; t < sizeof(modifiers)/sizeof(modifiers[0]); t++)
	  printf("%s\t\t%3d\n", modifiers[t].name, 1 << modifiers[t].bit);
}

static void
print_mod(int x) {
	unsigned int t;

	if (!x)
		printf("plain\t");
	else
	for (t = 0; t < sizeof(modifiers)/sizeof(modifiers[0]); t++)
	  if (x & (1 << modifiers[t].bit))
	    printf("%s\t", modifiers[t].name);
}

static void
print_bind(int bufj, int i, int j, char numeric) {
	if(j)
	    printf("\t");
	print_mod(j);
	printf("keycode %3d =", i);
	print_keysym(bufj, numeric);
	printf("\n");
}

#define DEFAULT		0
#define FULL_TABLE	1	/* one line for each keycode */
#define SEPARATE_LINES	2	/* one line for each (modifier,keycode) pair */
#define	UNTIL_HOLE	3	/* one line for each keycode, until 1st hole */

static void
dump_keys(char table_shape, char numeric) {
	int i, j, k;
	int buf[MAX_NR_KEYMAPS];
	int isletter, islatin, isasexpected;
	int typ, val;
	int alt_is_meta = 0;
	int zapped[MAX_NR_KEYMAPS];

	get_keymaps();
	print_keymaps();
	if (!keymapnr)
	  return;

	if (table_shape == FULL_TABLE || table_shape == SEPARATE_LINES)
	  goto no_shorthands;

	/* first pass: determine whether to set alt_is_meta */
	for (j = 0; j < MAX_NR_KEYMAPS; j++) {
	     int ja = (j | M_ALT);
	     if (j != ja && keymap_index[j] >= 0 && keymap_index[ja] >= 0)
		  for (i = 1; i < nr_keys; i++) {
		       int buf0, buf1, type;

		       buf0 = get_bind(i, j);
		       if (buf0 == -1)
			   break;
		       type = KTYP(buf0);
		       if ((type == KT_LATIN || type == KT_LETTER)
			   && KVAL(buf0) < 128) {
			    buf1 = get_bind(i, ja);
			    if (buf1 != K(KT_META, KVAL(buf0))) {
				 if (verbose) {
				      printf(_("# not alt_is_meta: "
		      		"on keymap %d key %d is bound to"),
					      ja, i);
				      print_keysym(buf1, numeric);
				      printf("\n");
				 }
				 goto not_alt_is_meta;
			    }
		       }
		  }
	}
	alt_is_meta = 1;
	printf("alt_is_meta\n");
not_alt_is_meta:

no_shorthands:
	for (i = 1; i < nr_keys; i++) {
	    for (j = 0; j < keymapnr; j++)
	      buf[j] = get_bind(i, good_keymap[j]);
	    if (buf[0] == -1)
		break;

	    if (table_shape == FULL_TABLE) {
		printf("keycode %3d =", i);
		for (j = 0; j < keymapnr; j++)
		  print_keysym(buf[j], numeric);
		printf("\n");
		continue;
	    }

	    if (table_shape == SEPARATE_LINES) {
		for (j = 0; j < keymapnr; j++)
		  print_bind(buf[j], i, good_keymap[j], numeric);
		printf("\n");
		continue;
	    }

	    typ = KTYP(buf[0]);
	    val = KVAL(buf[0]);
	    islatin = (typ == KT_LATIN || typ == KT_LETTER);
	    isletter = (islatin &&
			((val >= 'A' && val <= 'Z') ||
			 (val >= 'a' && val <= 'z')));
	    isasexpected = 0;
	    if (isletter) {
		u_short defs[16];
		defs[0] = K(KT_LETTER, val);
		defs[1] = K(KT_LETTER, val ^ 32);
		defs[2] = defs[0];
		defs[3] = defs[1];
		for(j=4; j<8; j++)
		  defs[j] = K(KT_LATIN, val & ~96);
		for(j=8; j<16; j++)
		  defs[j] = K(KT_META, KVAL(defs[j-8]));

		for(j = 0; j < keymapnr; j++) {
		    k = good_keymap[j];
		    if ((k >= 16 && buf[j] != K_HOLE) || (k < 16 && buf[j] != defs[k]))
		      goto unexpected;
		}
		isasexpected = 1;
	    }
	  unexpected:

	    /* wipe out predictable meta bindings */
	    for (j = 0; j < keymapnr; j++)
		    zapped[j] = 0;
	    if (alt_is_meta) {
		 for(j = 0; j < keymapnr; j++) {
		      int ka, ja, ktyp;
		      k = good_keymap[j];
		      ka = (k | M_ALT);
		      ja = keymap_index[ka];
		      if (k != ka && ja >= 0
		       && ((ktyp=KTYP(buf[j])) == KT_LATIN || ktyp == KT_LETTER)
		       && KVAL(buf[j]) < 128) {
			   if (buf[ja] != K(KT_META, KVAL(buf[j])))
				fprintf(stderr, _("impossible: not meta?\n"));
			   buf[ja] = K_HOLE;
			   zapped[ja] = 1;
		      }
		 }
	    }

	    printf("keycode %3d =", i);
	    if (isasexpected) {
		/* print only a single entry */
		/* suppress the + for ordinary a-zA-Z */
		print_keysym(K(KT_LATIN, val), numeric);
		printf("\n");
	    } else {
		/* choose between single entry line followed by exceptions,
		   and long line followed by exceptions; avoid VoidSymbol */
		int bad = 0;
		int count = 0;
		for(j = 1; j < keymapnr; j++) if (!zapped[j]) {
		    if (buf[j] != buf[0])
		      bad++;
		    if (buf[j] != K_HOLE)
		      count++;
		}
		if (bad <= count && bad < keymapnr-1) {
		    if (buf[0] != K_HOLE)
		      print_keysym(buf[0], numeric);
		    printf("\n");
		    for (j = 1; j < keymapnr; j++)
		      if (buf[j] != buf[0] && !zapped[j])
			print_bind(buf[j], i, good_keymap[j], numeric);
		} else {
		    for (j = 0; j < keymapnr && buf[j] != K_HOLE &&
				 (j == 0 || table_shape != UNTIL_HOLE ||
				  good_keymap[j] == good_keymap[j-1]+1); j++)
		      print_keysym(buf[j], numeric);
		    printf("\n");
		    for ( ; j < keymapnr; j++)
		      if (buf[j] != K_HOLE)
			print_bind(buf[j], i, good_keymap[j], numeric);
		}
	    }
	}
}

static void
dump_funcs(void) {
	int i;
	struct kbsentry fbuf;
	unsigned char *p;

	for (i = 0; i < MAX_NR_FUNC; i++) {
		fbuf.kb_func = i;
		if (ioctl(fd, KDGKBSENT, (unsigned long)&fbuf)) {
		    if (errno == EINVAL && i > 0) /* an old kernel */
		      break;
		    perror("KDGKBSENT");
		    fprintf(stderr, _("KDGKBSENT failed at index %d: "), i);
		    exit(1);
		}
		if (!fbuf.kb_string[0])
			continue;
		printf("string %s = \"", syms[KT_FN].table[i]);
		for (p = fbuf.kb_string; *p; p++) {
			if (*p == '"' || *p == '\\') {
				putchar('\\'); putchar(*p);
			} else if (isgraph(*p) || *p == ' ')
				putchar(*p);
			else
				printf("\\%03o", *p);
		}
		printf("\"\n");
	}
}

static void attr_noreturn
usage(void) {
	fprintf(stderr, _("dumpkeys version %s"), PACKAGE_VERSION);
	fprintf(stderr, _("\
\n\
usage: dumpkeys [options...]\n\
\n\
valid options are:\n\
\n\
	-h --help	    display this help text\n\
	-i --short-info	    display information about keyboard driver\n\
	-l --long-info	    display above and symbols known to loadkeys\n\
	-n --numeric	    display keytable in hexadecimal notation\n\
	-f --full-table	    don't use short-hand notations, one row per keycode\n\
	-1 --separate-lines one line per (modifier,keycode) pair\n\
	   --funcs-only	    display only the function key strings\n\
	   --keys-only	    display only key bindings\n\
	   --compose-only   display only compose key combinations\n\
	-c --charset="));
	list_charsets(stderr);
	fprintf(stderr, _("\
			    interpret character action codes to be from the\n\
			    specified character set\n\
"));
	exit(1);
}

int
main (int argc, char *argv[]) {
	const char *short_opts = "hilvsnf1S:c:V";
	const struct option long_opts[] = {
		{ "help",	no_argument,		NULL, 'h' },
		{ "short-info",	no_argument,		NULL, 'i' },
		{ "long-info",	no_argument,		NULL, 'l' },
		{ "numeric",	no_argument,		NULL, 'n' },
		{ "full-table",	no_argument,		NULL, 'f' },
		{ "separate-lines",no_argument,		NULL, '1' },
		{ "shape",	required_argument,	NULL, 'S' },
		{ "funcs-only",	no_argument,		NULL, 't' },
		{ "keys-only",	no_argument,		NULL, 'k' },
		{ "compose-only",no_argument,		NULL, 'd' },
		{ "charset",	required_argument,	NULL, 'c' },
		{ "verbose",	no_argument,		NULL, 'v' },
		{ "version",	no_argument,		NULL, 'V' },
		{ NULL,	0, NULL, 0 }
	};
	int c;
	char long_info = 0;
	char short_info = 0;
	char numeric = 0;
	char table_shape = 0;
	char funcs_only = 0;
	char keys_only = 0;
	char diac_only = 0;

	set_progname(argv[0]);

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE_NAME, LOCALEDIR);
	textdomain(PACKAGE_NAME);

	while ((c = getopt_long(argc, argv,
		short_opts, long_opts, NULL)) != -1) {
		switch (c) {
			case 'i':
				short_info = 1;
				break;
			case 's':
			case 'l':
				long_info = 1;
				break;
			case 'n':
				numeric = 1;
				break;
			case 'f':
				table_shape = FULL_TABLE;
				break;
			case '1':
				table_shape = SEPARATE_LINES;
				break;
			case 'S':
				table_shape = atoi(optarg);
				break;
			case 't':
				funcs_only = 1;
				break;
			case 'k':
				keys_only = 1;
				break;
			case 'd':
				diac_only = 1;
				break;
			case 'v':
				verbose = 1;
				break;
			case 'c':
				if ((set_charset(optarg)) != 0)
					usage();
				printf("charset \"%s\"\n", optarg);
				break;
			case 'V':
				print_version_and_exit();
			case 'h':
			case '?':
				usage();
		}
	}

	if (optind < argc)
		usage();

	fd = getfd(NULL);

	find_nr_keys();

	if (short_info || long_info) {
		show_short_info();
		if (long_info)
			dump_symbols();
		exit(0);
	}

#ifdef KDGKBDIACR
	if (!diac_only) {
#endif
	    if (!funcs_only)
		dump_keys(table_shape, numeric);
	    if (!keys_only)
		dump_funcs();
#ifdef KDGKBDIACR
	}
	if (!funcs_only && !keys_only)
		dump_diacs();
#endif

	exit(0);
}
