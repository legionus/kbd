#include "config.h"

#include <linux/keyboard.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "array_size.h"
#include "keymap.h"

#include "contextP.h"
#include "ksyms.h"

#include "syms.cp1250.h"
#include "syms.ethiopic.h"
#include "syms.iso8859_15.h"
#include "syms.iso8859_5.h"
#include "syms.iso8859_7.h"
#include "syms.iso8859_8.h"
#include "syms.iso8859_9.h"
#include "syms.koi8.h"
#include "syms.latin1.h"
#include "syms.latin2.h"
#include "syms.latin3.h"
#include "syms.latin4.h"
#include "syms.mazovia.h"
#include "syms.sami.h"
#include "syms.thai.h"

#include "syms.synonyms.h"

#include "syms.ktyp.h"

#ifndef MIN
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif

#define E(x) { x, ARRAY_SIZE(x) }

static const syms_entry syms[] = {
	E(iso646_syms), /* KT_LATIN */
	E(fn_syms),     /* KT_FN */
	E(spec_syms),   /* KT_SPEC */
	E(pad_syms),    /* KT_PAD */
	E(dead_syms),   /* KT_DEAD */
	E(cons_syms),   /* KT_CONS */
	E(cur_syms),    /* KT_CUR */
	E(shift_syms),  /* KT_SHIFT */
	{ NULL, 0 },    /* KT_META */
	E(ascii_syms),  /* KT_ASCII */
	E(lock_syms),   /* KT_LOCK */
	{ NULL, 0 },    /* KT_LETTER */
	E(sticky_syms), /* KT_SLOCK */
	{ NULL, 0 },    /* KT_DEAD2 */
	E(brl_syms)     /* KT_BRL */
};

#undef E

const int syms_size = ARRAY_SIZE(syms);
const int syn_size  = ARRAY_SIZE(synonyms);

typedef enum {
	CHARSET_ISO_8859_1,
	CHARSET_ISO_8859_2,
	CHARSET_ISO_8859_3,
	CHARSET_ISO_8859_4,
	CHARSET_ISO_8859_5,
	CHARSET_ISO_8859_7,
	CHARSET_ISO_8859_8,
	CHARSET_ISO_8859_9,
	CHARSET_ISO_8859_10,
	CHARSET_ISO_8859_15,
	CHARSET_MAZOVIA,
	CHARSET_CP_1250,
	CHARSET_KOI8_R,
	CHARSET_KOI8_U,
	CHARSET_TIS_620,
	CHARSET_ISO_10646_18,
	CHARSET_ISO_IR_197,
	CHARSET_ISO_IR_209,
	CHARSET_COUNTS,
} charset_index_t;

#define ENTRY(index, name, syms, start) \
	[index] = { name, syms, ARRAY_SIZE(syms), start }

static const struct cs {
	const char *charset;
	const sym *charnames;
	const int size;
	const unsigned short start;
} charsets[CHARSET_COUNTS] = {
	ENTRY(CHARSET_ISO_8859_1,   "iso-8859-1",   latin1_syms,      160),
	ENTRY(CHARSET_ISO_8859_2,   "iso-8859-2",   latin2_syms,      160),
	ENTRY(CHARSET_ISO_8859_3,   "iso-8859-3",   latin3_syms,      160),
	ENTRY(CHARSET_ISO_8859_4,   "iso-8859-4",   latin4_syms,      160),
	ENTRY(CHARSET_ISO_8859_5,   "iso-8859-5",   iso8859_5_syms,   160),
	ENTRY(CHARSET_ISO_8859_7,   "iso-8859-7",   iso8859_7_syms,   160),
	ENTRY(CHARSET_ISO_8859_8,   "iso-8859-8",   iso8859_8_syms,   160),
	ENTRY(CHARSET_ISO_8859_9,   "iso-8859-9",   iso8859_9_syms,   160),
	ENTRY(CHARSET_ISO_8859_10,  "iso-8859-10",  latin6_syms,      160),
	ENTRY(CHARSET_ISO_8859_15,  "iso-8859-15",  iso8859_15_syms,  160),
	ENTRY(CHARSET_MAZOVIA,      "mazovia",      mazovia_syms,     128),
	ENTRY(CHARSET_CP_1250,      "cp-1250",      cp1250_syms,      128),
	ENTRY(CHARSET_KOI8_R,       "koi8-r",       koi8_syms,        128),
	ENTRY(CHARSET_KOI8_U,       "koi8-u",       koi8_syms,        128),
	ENTRY(CHARSET_TIS_620,      "tis-620",      tis_620_syms,     160), /* thai */
	ENTRY(CHARSET_ISO_10646_18, "iso-10646-18", iso10646_18_syms, 159), /* ethiopic */
	ENTRY(CHARSET_ISO_IR_197,   "iso-ir-197",   iso_ir_197_syms,  160), /* sami */
	ENTRY(CHARSET_ISO_IR_209,   "iso-ir-209",   iso_ir_209_syms,  160), /* sami */
};

#undef ENTRY

/* Functions for both dumpkeys and loadkeys. */

void lk_list_charsets(FILE *f)
{
	size_t lth;
	int ct;
	unsigned int i, j;
	const char *mm[] = { "iso-8859-", "koi8-" };

	for (j = 0; j < ARRAY_SIZE(mm); j++) {
		if (j)
			fprintf(f, ",");
		fprintf(f, "%s{", mm[j]);
		ct  = 0;
		lth = strlen(mm[j]);
		for (i = 1; i < ARRAY_SIZE(charsets); i++) {
			if (!strncmp(charsets[i].charset, mm[j], lth)) {
				if (ct++)
					fprintf(f, ",");
				fprintf(f, "%s", charsets[i].charset + lth);
			}
		}
		fprintf(f, "}");
	}
	for (i = 0; i < ARRAY_SIZE(charsets); i++) {
		for (j = 0; j < ARRAY_SIZE(mm); j++) {
			lth = strlen(mm[j]);
			if (!strncmp(charsets[i].charset, mm[j], lth))
				goto nxti;
		}
		fprintf(f, ",%s", charsets[i].charset);
	nxti:;
	}
	fprintf(f, "\n");
}

const char *
lk_get_charset(struct lk_ctx *ctx)
{
	if (!ctx || ctx->charset >= ARRAY_SIZE(charsets))
		return NULL;

	return charsets[ctx->charset].charset;
}

int lk_set_charset(struct lk_ctx *ctx, const char *charset)
{
	unsigned short i;

	for (i = 0; i < ARRAY_SIZE(charsets); i++) {
		if (!strcasecmp(charsets[i].charset, charset)) {
			ctx->charset = i;
			return 0;
		}
	}
	return 1;
}

int
get_sym_size(struct lk_ctx *ctx, int ktype)
{
	if (ktype >= syms_size) {
		ERR(ctx, _("unable to get symbol by wrong type: %d"), ktype);
		return 0;
	}

	return syms[ktype].size;
}

const char *
get_sym(struct lk_ctx *ctx, int ktype, int index)
{
	if (!get_sym_size(ctx, ktype))
		return NULL;

	if (index >= syms[ktype].size) {
		ERR(ctx, _("unable to get symbol of %d type by wrong index: %d"), ktype, index);
		return NULL;
	}

	return syms[ktype].table[index];
}

char *
lk_get_sym(struct lk_ctx *ctx, int ktype, int index)
{
	const char *ksym = get_sym(ctx, ktype, index);
	return (ksym ? strdup(ksym) : NULL);
}

const char *
codetoksym(struct lk_ctx *ctx, int code)
{
	unsigned int i;
	int j;
	const sym *p;

	if (code < 0)
		return NULL;

	if (code < 0x1000) { /* "traditional" keysym */
		if (code < 0x80)
			return get_sym(ctx, KT_LATIN, code);

		if (KTYP(code) == KT_META)
			return NULL;

		if (KTYP(code) == KT_LETTER)
			code = K(KT_LATIN, KVAL(code));

		if (KTYP(code) > KT_LATIN)
			return get_sym(ctx, KTYP(code), KVAL(code));

		if (KVAL(code) >= charsets[ctx->charset].start) {
			j = KVAL(code) - charsets[ctx->charset].start;

			if (j >= 0 && j < charsets[ctx->charset].size) {
				p = charsets[ctx->charset].charnames + j;

				if (p->name[0])
					return p->name;
			}
		}
	}

	else { /* Unicode keysym */
		code = U(code);

		if (code < 0x80)
			return get_sym(ctx, KT_LATIN, code);

		for (i = 0; i < ARRAY_SIZE(charsets); i++) {
			p = charsets[i].charnames;

			for (j = 0; j < charsets[i].size; j++, p++) {
				if (p->uni == code && p->name[0])
					return p->name;
			}
		}
	}

	return NULL;
}

char *
lk_code_to_ksym(struct lk_ctx *ctx, int code)
{
	const char *s;

	s = codetoksym(ctx, code);
	if (!s)
		return NULL;

	return strdup(s);
}

/* Functions for loadkeys. */

static int
kt_latin(struct lk_ctx *ctx, const char *s, int direction)
{
	int i, max;

	if (direction != TO_UNICODE) {
		const sym *p;

		p = charsets[ctx->charset].charnames;
		max = charsets[ctx->charset].size;

		for (i = 0; i < max; i++, p++) {
			if (p->name[0] && !strcmp(s, p->name))
				return K(KT_LATIN, charsets[ctx->charset].start + i);
		}
	}

	max = (direction == TO_UNICODE
			? MIN(UNICODE_ASCII_LEN, syms[KT_LATIN].size)
			: syms[KT_LATIN].size);

	for (i = 0; i < max; i++) {
		if (!strcmp(s, get_sym(ctx, KT_LATIN, i)))
			return K(KT_LATIN, i);
	}

	return -1;
}

static unsigned short get_charset_uni(charset_index_t index, const char *s)
{
	const sym *p = charsets[index].charnames;

	for (int i = 0; i < charsets[index].size; i++, p++) {
		if (p->name[0] && !strcmp(s, p->name))
			return p->uni;
	}

	return 0;
}

int ksymtocode(struct lk_ctx *ctx, const char *s, int direction)
{
	unsigned short i, uni;
	int n, keycode;

	if (direction == TO_AUTO)
		direction = (ctx->flags & LK_FLAG_PREFER_UNICODE)
		                ? TO_UNICODE
		                : TO_8BIT;

	if (!strncmp(s, "Meta_", 5)) {
		keycode = ksymtocode(ctx, s + 5, TO_8BIT);
		if (KTYP(keycode) == KT_LATIN)
			return K(KT_META, KVAL(keycode));

		/* Avoid error messages for Meta_acute with UTF-8 */
		else if (direction == TO_UNICODE)
			return (0);

		/* fall through to error printf */
	}

	if (!strncmp(s, "dead2_", 6)) {
		keycode = ksymtocode(ctx, s + 6, TO_8BIT);
		if (KTYP(keycode) == KT_LATIN)
			return K(KT_DEAD2, KVAL(keycode));

		/* fall through to error printf */
	}

	if ((n = kt_latin(ctx, s, direction)) >= 0) {
		return n;
	}

	for (i = 1; i < syms_size; i++) {
		for (int j = 0; j < syms[i].size; j++) {
			const char *ksym = get_sym(ctx, i, j);
			if (ksym && !strcmp(s, ksym))
				return K(i, j);
		}
	}

	for (i = 0; i < syn_size; i++)
		if (!strcmp(s, synonyms[i].synonym))
			return ksymtocode(ctx, synonyms[i].official_name, direction);

	if (direction == TO_UNICODE) {
		if ((uni = get_charset_uni(ctx->charset, s)) > 0)
			return U(uni);

		/* not found in the current charset, maybe we'll have good luck in others? */
		for (i = 0; i < ARRAY_SIZE(charsets); i++) {
			if (i == ctx->charset) {
				continue;
			}

			if ((uni = get_charset_uni(i, s)) > 0)
				return U(uni);
		}
	} else /* if (!chosen_charset[0]) */ {
		/* note: some keymaps use latin1 but with euro,
		   so set_charset() would fail */
		/* note: some keymaps with charset line still use
		   symbols from more than one character set,
		   so we cannot have the  `if (!chosen_charset[0])'  here */
		charset_index_t try_charset[] = {
			CHARSET_ISO_8859_1, CHARSET_ISO_8859_15,
			CHARSET_ISO_8859_2, CHARSET_ISO_8859_3,
			CHARSET_ISO_8859_4
		};

		for (size_t num = 0; num < ARRAY_SIZE(try_charset); num++) {
			const struct cs *cs = charsets + try_charset[num];
			const sym *p = cs->charnames;

			for (i = 0; i < cs->size; i++, p++) {
				if (p->name[0] && !strcmp(s, p->name)) {
					INFO(ctx, _("assuming %s %s"), cs->charset, s);
					return K(KT_LATIN, cs->start + i);
				}
			}
		}
	}

	ERR(ctx, _("unknown keysym '%s'"), s);

	return CODE_FOR_UNKNOWN_KSYM;
}

int lk_ksym_to_unicode(struct lk_ctx *ctx, const char *s)
{
	return ksymtocode(ctx, s, TO_UNICODE);
}

int convert_code(struct lk_ctx *ctx, int code, int direction)
{
	const char *ksym;
	int unicode_forced   = (direction == TO_UNICODE);
	int input_is_unicode = (code >= 0x1000);
	int result;

	if (direction == TO_AUTO)
		direction = (ctx->flags & LK_FLAG_PREFER_UNICODE)
		                ? TO_UNICODE
		                : TO_8BIT;

	if (KTYP(code) == KT_META)
		return code;
	else if (!input_is_unicode && code < 0x80)
		/* basic ASCII is fine in every situation */
		return code;
	else if (input_is_unicode && U(code) < 0x80)
		/* so is Unicode "Basic Latin" */
		return U(code);
	else if ((input_is_unicode && direction == TO_UNICODE) ||
	         (!input_is_unicode && direction == TO_8BIT))
		/* no conversion necessary */
		result = code;
	else {
		/* depending on direction, this will give us either an 8-bit
		 * K(KTYP, KVAL) or a Unicode keysym xor UNICODE_MASK */
		ksym = codetoksym(ctx, code);
		if (ksym)
			result = ksymtocode(ctx, ksym, direction);
		else
			result = code;
		if (direction == TO_UNICODE && KTYP(code) == KT_LETTER && U(result) < 0x100) {
			/* Unicode Latin-1 Supplement */
			result = K(KT_LETTER, U(result));
		}
	}

	/* if direction was TO_UNICODE from the beginning, we return the true
	 * Unicode value (without the UNICODE_MASK) */
	if (unicode_forced && result >= 0x1000)
		return U(result);
	else
		return result;
}

int add_capslock(struct lk_ctx *ctx, int code)
{
	if (KTYP(code) == KT_LATIN && (!(ctx->flags & LK_FLAG_PREFER_UNICODE) || code < 0x80))
		return K(KT_LETTER, KVAL(code));
	else if (U(code) < 0x100)
		/* Unicode Latin-1 Supplement */
		/* a bit dirty to use KT_LETTER here, but it should work */
		return K(KT_LETTER, U(code));
	else
		return convert_code(ctx, code, TO_AUTO);
}


int lk_convert_code(struct lk_ctx *ctx, int code, int direction)
{
	if (!ctx)
		return CODE_FOR_UNKNOWN_KSYM;
	return convert_code(ctx, code, direction);
}

int lk_add_capslock(struct lk_ctx *ctx, int code)
{
	if (!ctx)
		return CODE_FOR_UNKNOWN_KSYM;
	return add_capslock(ctx, code);
}
