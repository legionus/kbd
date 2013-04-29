/* parser.y
 *
 * This file is part of kbd project.
 * Copyright (C) 1993  Risto Kankkunen.
 * Copyright (C) 1993  Eugene G. Crosser.
 * Copyright (C) 1994-2007  Andries E. Brouwer.
 * Copyright (C) 2007-2013  Alexey Gladkov <gladkov.alexey@gmail.com>
 *
 * This file is covered by the GNU General Public License,
 * which should be included with kbd as the file COPYING.
 */
%{
#define YY_HEADER_EXPORT_START_CONDITIONS 1

#include "nls.h"
#include "kbd.h"

#include "ksyms.h"
#include "modifiers.h"

#include "parser.h"
#include "analyze.h"
%}

%code requires {
#include "keymap.h"

#ifndef STRDATA_STRUCT
#define STRDATA_STRUCT
#define MAX_PARSER_STRING 512
struct strdata {
	unsigned int len;
	unsigned char data[MAX_PARSER_STRING];
};
#endif
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
static int
yyerror(yyscan_t scanner __attribute__ ((unused)),
        struct keymap *kmap, const char *s)
{
	ERR(kmap, "%s", s);
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

			if (lk_add_func(kmap, ke) == -1)
				return -1;
		}
	}
	return 0;
}

static int
compose_as_usual(struct keymap *kmap, char *charset)
{
	if (charset && strcmp(charset, "iso-8859-1")) {
		ERR(kmap, _("loadkeys: don't know how to compose for %s"), charset);
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
			if (lk_add_compose(kmap, ptr.c1, ptr.c2, ptr.c3) == -1)
				return -1;
		}
	}
	return 0;
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
				if (lk_set_charset(kmap, (char *) $2.data)) {
					ERR(kmap,
						_("unknown charset %s - ignoring charset request\n"),
						(char *) $2.data);
					YYERROR;
				}
				kmap->keywords |= LK_KEYWORD_CHARSET;

				/* Unicode: The first 256 code points were made
				   identical to the content of ISO 8859-1 */
				if (kmap->prefer_unicode &&
				    !strcasecmp((char *) $2.data, "iso-8859-1"))
					kmap->prefer_unicode = 0;
			}
		;
altismetaline	: ALT_IS_META EOL
			{
				kmap->keywords |= LK_KEYWORD_ALTISMETA;
			}
		;
usualstringsline: STRINGS AS USUAL EOL
			{
				if (strings_as_usual(kmap) == -1)
					YYERROR;
				kmap->keywords |= LK_KEYWORD_STRASUSUAL;
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
				kmap->keywords |= LK_KEYWORD_KEYMAPS;
			}
		;
range		: range COMMA range0
		| range0
		;
range0		: NUMBER DASH NUMBER
			{
				int i;
				for (i = $1; i <= $3; i++) {
					if (lk_add_map(kmap, i) == -1)
						YYERROR;
				}
			}
		| NUMBER
			{
				if (lk_add_map(kmap, $1) == -1)
					YYERROR;
			}
		;
strline		: STRING LITERAL EQUALS STRLITERAL EOL
			{
				struct kbsentry ke;

				if (KTYP($2) != KT_FN) {
					ERR(kmap, _("'%s' is not a function key symbol"),
						syms[KTYP($2)].table[KVAL($2)]);
					YYERROR;
				}

				ke.kb_func = KVAL($2);
				strncpy((char *) ke.kb_string,
				        (char *) $4.data,
				        sizeof(ke.kb_string));
				ke.kb_string[sizeof(ke.kb_string) - 1] = 0;

				if (lk_add_func(kmap, ke) == -1)
					YYERROR;
			}
		;
compline        : COMPOSE compsym compsym TO compsym EOL
                        {
				if (lk_add_compose(kmap, $2, $3, $5) == -1)
					YYERROR;
			}
		 | COMPOSE compsym compsym TO rvalue EOL
			{
				if (lk_add_compose(kmap, $2, $3, $5) == -1)
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
				if (lk_add_key(kmap, kmap->mod, $4, $6) < 0)
					YYERROR;
			}
		| PLAIN KEYCODE NUMBER EQUALS rvalue EOL
			{
				if (lk_add_key(kmap, 0, $3, $5) < 0)
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

						if (lk_del_key(kmap, j, $2) == -1)
							YYERROR;
					}
				}

				if (kmap->keywords & LK_KEYWORD_KEYMAPS) {
					i = 0;

					for (j = 0; j < kmap->max_keymap; j++) {
						if (!(kmap->defining[j]))
							continue;

						if (kmap->rvalct != 1 || i == 0) {
							keycode = (i < kmap->rvalct)
								? kmap->key_buf[i]
								: K_HOLE;

							if (lk_add_key(kmap, j, $2, keycode) < 0)
								YYERROR;
						}
						i++;
					}

					if (i < kmap->rvalct) {
						ERR(kmap, _("too many (%d) entries on one line"),
							kmap->rvalct);
						YYERROR;
					}
				} else {
					for (i = 0; i < kmap->rvalct; i++) {
						if (lk_add_key(kmap, i, $2, kmap->key_buf[i]) < 0)
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
					ERR(kmap, _("too many key definitions on one line"));
					YYERROR;
				}
				kmap->key_buf[kmap->rvalct++] = $1;
			}
		;
rvalue		: NUMBER	{ $$ = convert_code(kmap, $1, TO_AUTO);		}
                | PLUS NUMBER	{ $$ = add_capslock(kmap, $2);			}
		| UNUMBER	{ $$ = convert_code(kmap, $1^0xf000, TO_AUTO);	}
		| PLUS UNUMBER	{ $$ = add_capslock(kmap, $2^0xf000);		}
		| LITERAL	{ $$ = $1;					}
                | PLUS LITERAL	{ $$ = add_capslock(kmap, $2);			}
		;
%%

int
lk_parse_keymap(struct keymap *kmap, lkfile_t *f)
{
	yyscan_t scanner;
	int rc = -1;

	yylex_init(&scanner);
	yylex_init_extra(kmap, &scanner);

	INFO(kmap, _("Loading %s"), f->pathname);

	if (stack_push(kmap, f, scanner) == -1)
		goto fail;

	if (yyparse(scanner, kmap))
		goto fail;

	rc = 0;

	stack_pop(kmap, scanner);

 fail:	yylex_destroy(scanner);
	return rc;
}
