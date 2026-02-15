%{
#include "config.h"

#include <stdlib.h>
#include <unistd.h> /* readlink */

#include <kbdfile.h>

#include "contextP.h"
#include "ksyms.h"

#include "parser.h"
%}

%top {
#include "keymap.h"
int stack_push(struct lk_ctx *ctx, struct kbdfile *fp, void *scanner);
int stack_pop(struct lk_ctx *ctx, void *scanner);
}

%option reentrant
%option bison-bridge
%option stack
%option never-interactive
%option noyywrap
%option nounput
%option noinput
%option noyy_top_state

%option extra-type="struct lk_ctx *"

%{
int
stack_push(struct lk_ctx *ctx, struct kbdfile *fp, void *scanner)
{
	int i = 0;

	while (i < MAX_INCLUDE_DEPTH && ctx->stack[i]) i++;

	if (i == MAX_INCLUDE_DEPTH) {
		ERR(ctx, _("includes are nested too deeply"));
		return -1;
	}

	ctx->stack[i] = fp;

	yypush_buffer_state(yy_create_buffer(kbdfile_get_file(fp), YY_BUF_SIZE, scanner), scanner);
	return 0;
}

int
stack_pop(struct lk_ctx *ctx, void *scanner)
{
	int i = 0;

	while (i < MAX_INCLUDE_DEPTH && ctx->stack[i]) i++;
	if (!i)
		return 0;
	i--;

	/*
	 * The top of stack is input file for library. No need to close it.
	 */
	if (i) {
		kbdfile_free(ctx->stack[i]);
	}
	ctx->stack[i] = NULL;

	yypop_buffer_state(scanner);
	return 0;
}

/*
 * Where shall we look for an include file?
 * Current strategy (undocumented, may change):
 *
 * 1. Look for a user-specified LOADKEYS_INCLUDE_PATH
 * 2. Try . and ../include and ../../include
 * 3. Try D and D/../include and D/../../include
 *    where D is the directory from where we are loading the current file.
 * 4. Try KD/include and KD/#/include where KD = DATADIR/KEYMAPDIR.
 *
 * Expected layout:
 * KD has subdirectories amiga, atari, i386, mac, sun, include
 * KD/include contains architecture-independent stuff
 * like strings and iso-8859-x compose tables.
 * KD/i386 has subdirectories qwerty, ... and include;
 * this latter include dir contains stuff with keycode=...
 *
 * (Of course, if the present setup turns out to be reasonable,
 * then later also the other architectures will grow and get
 * subdirectories, and the hard-coded i386 below will go again.)
 *
 * People that dislike a dozen lookups for loadkeys
 * can easily do "loadkeys file_with_includes; dumpkeys > my_keymap"
 * and afterwards use only "loadkeys /fullpath/mykeymap", where no
 * lookups are required.
 */
static const char *const include_dirpath0[] = {
	"",
	NULL
};
static const char *const include_dirpath1[] = {
	"../include/",
	"../../include/",
	NULL
};
static const char *const include_dirpath3[] = {
	DATADIR "/" KEYMAPDIR "/include/",
	DATADIR "/" KEYMAPDIR "/i386/include/",
	DATADIR "/" KEYMAPDIR "/mac/include/",
	NULL
};

static const char *const include_suffixes[] = {
	"",
	".inc",
	NULL
};

#define INCL_1_LEVEL_UPPER "../include/"
#define INCL_2_LEVEL_UPPER "../../include/"

static int find_incl_file_near_fn(struct lk_ctx *ctx, char *filename, char *path,
		struct kbdfile *fp)
{
	char *slash, *t0, *t1, *t2;
	size_t len;
	int rc;

	if (!path || !(slash = strrchr(path, '/')))
		return 1;

	len = (size_t) (slash - path + 1);

	t0 = strndup(path, len);
	t1 = malloc(len + sizeof(INCL_1_LEVEL_UPPER));
	t2 = malloc(len + sizeof(INCL_2_LEVEL_UPPER));

	if (t0 && t1 && t2) {
		const char *include_dirpath2[] = { t0, t1, t2, NULL };

		snprintf(t1, len + sizeof(INCL_1_LEVEL_UPPER), "%s%s", t0, INCL_1_LEVEL_UPPER);
		snprintf(t2, len + sizeof(INCL_2_LEVEL_UPPER), "%s%s", t0, INCL_2_LEVEL_UPPER);

		rc = kbdfile_find(filename, include_dirpath2, include_suffixes, fp);
	} else {
		ERR(ctx, _("out of memory"));
		rc = -1;
	}

	free(t0);
	free(t1);
	free(t2);

	return rc;
}

static int find_standard_incl_file(struct lk_ctx *ctx, char *filename,
		struct kbdfile *fp)
{
	char *pathname;
	int rc = 1;
	int i = 0;

	while (i < MAX_INCLUDE_DEPTH && ctx->stack[i]) i++;
	if (i == 0)
		return -1;
	i--;
	pathname = kbdfile_get_pathname(ctx->stack[i]);

	rc = kbdfile_find(filename, include_dirpath1, include_suffixes, fp);
	if (rc) {
		rc = find_incl_file_near_fn(ctx, filename, pathname, fp);
		if (rc == -1)
			return rc;
	}

	/* If filename is a symlink, also look near its target. */
	if (rc) {
		char buf[MAXPATHLEN], path[MAXPATHLEN], *ptr;
		ssize_t n;

		n = readlink(pathname, buf, sizeof(buf));
		if (n > 0 && n < (ssize_t) sizeof(buf)) {
			buf[n] = 0;
			if (buf[0] == '/') {
				rc = find_incl_file_near_fn(ctx, filename, buf, fp);

			} else if (strlen(pathname) + (size_t) n < sizeof(path)) {
				strlcpy(path, pathname, sizeof(path));
				ptr = strrchr(path, '/');
				if (ptr)
					ptr[1] = 0;
				strlcat(path, buf, sizeof(path));

				rc = find_incl_file_near_fn(ctx, filename, path, fp);
			}
		}
	}

	if (rc)
		rc = kbdfile_find(filename, include_dirpath3, include_suffixes, fp);

	return rc;
}

static int find_incl_file(struct lk_ctx *ctx, char *filename, struct kbdfile *fp)
{
	char *ev;

	if (!filename || !*filename)
		return 1;

	if (*filename == '/')		/* no path required */
		return kbdfile_find(filename, include_dirpath0, include_suffixes, fp);

	if ((ev = getenv("LOADKEYS_INCLUDE_PATH")) != NULL) {
		/* try user-specified path */
		char *path, *p;
		const char *user_dir[2] = { NULL, NULL };
		path = strdup(ev);
		if (!path) {
			ERR(ctx, _("out of memory"));
			return -1;
		}

		p = path;
		while (p) {
			int rc;
			char *t = strchr(p, ':');

			if (t)
				*t = 0;

			user_dir[0] = p;
			if (*p)
				rc = kbdfile_find(filename, user_dir, include_suffixes, fp);
			else	/* empty string denotes system path */
				rc = find_standard_incl_file(ctx, filename, fp);

			if (rc <= 0) {
				free(path);
				return rc;
			}

			p = t ? t + 1 : NULL;
		}

		free(path);
		return 1;
	}
	return find_standard_incl_file(ctx, filename, fp);
}

static struct kbdfile *open_include(struct lk_ctx *ctx, char *filename)
{
	int rc;
	struct kbdfile *fp;

	INFO(ctx, _("switching to %s"), filename);

	fp = kbdfile_new(ctx->kbdfile_ctx);
	if (!fp) {
		ERR(ctx, _("out of memory"));
		return NULL;
	}

	rc = find_incl_file(ctx, filename, fp);
	if (rc) {
		if (rc > 0)
			ERR(ctx, _("cannot open include file %s"), filename);
		return NULL;
	}

	return fp;
}

static int parse_positive_long(struct lk_ctx *ctx, char *text, char *value,
		int base, long long max, long *res)
{
	long v;

	errno = 0;
	v = strtol(value, NULL, base);

	if (errno) {
		if (errno == ERANGE && (v == LONG_MIN || v == LONG_MAX)) {
			ERR(ctx, "resulting value is out of range: %s", text);
			return -1;
		}
		ERR(ctx, _("unable to parse number: %s"), text);
		return -1;
	}

	if (v < 0) {
		ERR(ctx, _("value must be a positive number: %s"), text);
		return -1;
	}

	if (v > max) {
		ERR(ctx, _("value must be less than %lld: %s"), max, text);
		return -1;
	}

	*res = v;
	return 0;
}

static int parse_int(struct lk_ctx *ctx, char *text, char *value, int base, int *res)
{
	long v;

	if (!res || parse_positive_long(ctx, text, value, base, INT_MAX, &v) < 0)
		return -1;
	*res = (int) v;
	return 0;
}

static int parse_uchar(struct lk_ctx *ctx, char *text, char *value, int base, unsigned char *res)
{
	long v;

	if (!res || parse_positive_long(ctx, text, value, base, UCHAR_MAX, &v) < 0)
		return -1;
	*res = (unsigned char) v;
	return 0;
}

static int strdata_append(struct lk_ctx *ctx, struct string *string,
		const char *partstr, size_t partlen)
{
	char *tail;

	if ((string->str_len + partlen) >= sizeof(string->str_data)) {
		ERR(ctx, _("string too long"));
		return -1;
	}

	tail = string->str_data + string->str_len;

	if (partlen > 1) {
		strncpy(tail, partstr, partlen);
	} else {
		*tail = partstr[0];
	}
	string->str_len += partlen;

	return 0;
}

%}
%s RVALUE
%x STR
%x INCLSTR
Comment			#|!
Continuation		\\\n
Eol			\n
Blank			[ \t]
Include			include[ \t]*
Decimal			[1-9][0-9]*
Octal			0[0-7]*
Hex			0[xX][0-9a-fA-F]+
Unicode			U\+([0-9a-fA-F]){4,6}
Literal			[a-zA-Z][a-zA-Z_0-9]*
Octa			([0-7]){1,3}
Charset			charset|Charset|CharSet|CHARSET
Keymaps			keymaps|Keymaps|KeyMaps|KEYMAPS
Keycode			keycode|Keycode|KeyCode|KEYCODE
String			string|String|STRING
Equals			=
Plain			plain|Plain|PLAIN
Shift			shift|Shift|SHIFT
Control			control|Control|CONTROL
Alt			alt|Alt|ALT
AltGr			altgr|Altgr|AltGr|ALTGR
ShiftL			shiftl|ShiftL|SHIFTL
ShiftR			shiftr|ShiftR|SHIFTR
CtrlL			ctrll|CtrlL|CTRLL
CtrlR			ctrlr|CtrlR|CTRLR
CapsShift		capsshift|Capsshift|CapsShift|CAPSSHIFT
AltIsMeta		[aA][lL][tT][-_][iI][sS][-_][mM][eE][tT][aA]
Strings			strings|Strings|STRINGS
Compose			compose|Compose|COMPOSE
As			as|As|AS
Usual			usual|Usual|USUAL
For			for|For|FOR
On			on|On|ON
To                      to|To|TO

%%

{Include}		{
				yy_push_state(INCLSTR, yyscanner);
			}
<INCLSTR>\"[^\"\n]+\"	{
				struct kbdfile *fp;

				char *filename = strndup(yytext + 1, strlen(yytext) - 2);
				if (filename == NULL) {
					ERR(yyextra, _("out of memory"));
					return(ERROR);
				}

				fp = open_include(yyextra, filename);
				free(filename);

				if (!fp)
					return(ERROR);

				stack_push(yyextra, fp, yyscanner);

				while (((struct yyguts_t*)yyscanner)->yy_start_stack_ptr) {
					yy_pop_state(yyscanner);
				}
			}
<INCLSTR>[^"]|\"\"|\"[^"\n]*{Eol}	{
				ERR(yyextra, _("expected filename between quotes"));
				return(ERROR);
			}
<<EOF>>			{
				stack_pop(yyextra, yyscanner);
				if (!YY_CURRENT_BUFFER)
					yyterminate();
			}
{Continuation}		{
				yyset_lineno(yyget_lineno(yyscanner) + 1, yyscanner);
			}
{Eol}			{
				yyset_lineno(yyget_lineno(yyscanner) + 1, yyscanner);

				while (((struct yyguts_t*)yyscanner)->yy_start_stack_ptr) {
					yy_pop_state(yyscanner);
				}
				return(EOL);
			}
{Blank}+		; /* do nothing */
{Comment}.*/{Eol}	; /* do nothing */
{Equals}		{
				yy_push_state(RVALUE, yyscanner);
				lk_array_empty(yyextra->key_line);
				return(EQUALS);
			}
{String}		{
				yy_push_state(RVALUE, yyscanner);
				return(STRING);
			}
{To}			{
				yy_push_state(RVALUE, yyscanner);
				return(TO);
			}
{Unicode}		{
				if (parse_int(yyextra, yytext, yytext + 1, 16, &(yylval->num)) < 0)
					return(ERROR);

				if (yylval->num >= UNICODE_MASK) {
					ERR(yyextra, _("unicode keysym out of range: %s"),
						yytext);
					return(ERROR);
				}

				return(UNUMBER);
			}
{Decimal}|{Octal}|{Hex}	{
				if (parse_int(yyextra, yytext, yytext, 0, &(yylval->num)) < 0)
					return(ERROR);

				return(NUMBER);
			}
<RVALUE>{Literal}	{	return((yylval->num = ksymtocode(yyextra, yytext, TO_AUTO)) == -1 ? ERROR : LITERAL);	}
\-			{	return(DASH);		}
\,			{	return(COMMA);		}
\+			{	return(PLUS);		}
{Charset}		{	return(CHARSET);	}
{Keymaps}		{	return(KEYMAPS);	}
{Keycode}		{	return(KEYCODE);	}
{Plain}			{	return(PLAIN);		}
{Shift}			{	return(SHIFT);		}
{Control}		{	return(CONTROL);	}
{Alt}			{	return(ALT);		}
{AltGr}			{	return(ALTGR);		}
{ShiftL}		{	return(SHIFTL);		}
{ShiftR}		{	return(SHIFTR);		}
{CtrlL}			{	return(CTRLL);		}
{CtrlR}			{	return(CTRLR);		}
{CapsShift}		{	return(CAPSSHIFT);	}
{AltIsMeta}		{	return(ALT_IS_META);	}
{Strings}		{	return(STRINGS);	}
{Compose}		{	return(COMPOSE);	}
{As}			{	return(AS);		}
{Usual}			{	return(USUAL);		}
{On}			{	return(ON);		}
{For}			{	return(FOR);		}
'\\{Octa}'              {
				if (parse_int(yyextra, yytext, yytext + 2, 8, &(yylval->num)) < 0)
					return(ERROR);

				return(CCHAR);
			}
'\\.'                   {
				yylval->num = (unsigned char) yytext[2];
				return(CCHAR);
			}
'.'                     {
				yylval->num = (unsigned char) yytext[1];
				return(CCHAR);
			}
\"			{
				yylval->str.str_data[0] = '\0';
				yylval->str.str_len = 0;

				yy_push_state(STR, yyscanner);
			}
<STR>\\{Octa}		{
				unsigned char ch;

				if (parse_uchar(yyextra, yytext, yytext + 1, 8, &ch) < 0)
					return(ERROR);

				if (strdata_append(yyextra, &yylval->str, (char *) &ch, 1) < 0)
					return(ERROR);
			}
<STR>\\\"               {
				if (strdata_append(yyextra, &yylval->str, "\"", 1) < 0)
					return(ERROR);
			}
<STR>\\\\               {
				if (strdata_append(yyextra, &yylval->str, "\\", 1) < 0)
					return(ERROR);
			}
<STR>\\n		{
				if (strdata_append(yyextra, &yylval->str, "\n", 1) < 0)
					return(ERROR);
			}
<STR>[^\"\\]*		{
				size_t len = strlen(yytext);

				if (strdata_append(yyextra, &yylval->str, yytext, len) < 0)
					return(ERROR);
			}
<STR>\"			{
				strdata_append(yyextra, &yylval->str, "\0", 0);

				while (((struct yyguts_t*)yyscanner)->yy_start_stack_ptr) {
					yy_pop_state(yyscanner);
				}
				return(STRLITERAL);
			}
.			{
				return(ERROR);
			}
%%
