%{
#include <stdlib.h>
#include <unistd.h> /* readlink */

#include "ksyms.h"
#include "nls.h"
#include "kbd.h"
#include "paths.h"
#include "findfile.h"

#include "loadkeys.h"
#include "loadkeys.keymap.h"
%}

%option reentrant
%option bison-bridge
%option stack
%option yylineno
%option never-interactive
%option noyywrap
%option nounput
%option noinput
%option noyy_top_state

%option extra-type="struct keymap *"

%{
extern int lkverbose(struct keymap *kmap, int level, const char *fmt, ...);

int stack_push(struct keymap *kmap, lkfile_t *fp, yyscan_t scanner);
int stack_pop(struct keymap *kmap, yyscan_t scanner);

int
stack_push(struct keymap *kmap, lkfile_t *fp, yyscan_t scanner)
{
	int i = 0;

	while (kmap->stack[i]) i++;

	if (i == MAX_INCLUDE_DEPTH) {
		snprintf(kmap->errmsg, sizeof(kmap->errmsg),
			_("includes are nested too deeply"));
		return -1;
	}

	kmap->stack[i] = malloc(sizeof(lkfile_t));
	if (kmap->stack[i] == NULL) {
		snprintf(kmap->errmsg, sizeof(kmap->errmsg),
			_("out of memory"));
		return -1;
	}

	kmap->stack[i]->fd   = fp->fd;
	kmap->stack[i]->pipe = fp->pipe;
	strncpy(kmap->stack[i]->pathname, fp->pathname, sizeof(kmap->stack[i]->pathname) - 1);
	kmap->stack[i]->pathname[sizeof(kmap->stack[i]->pathname)] = '\0';

	yypush_buffer_state(yy_create_buffer(fp->fd, YY_BUF_SIZE, scanner), scanner);
	return 0;
}

int
stack_pop(struct keymap *kmap, yyscan_t scanner)
{
	int i = 0;

	while (kmap->stack[i]) i++;
	if (i == 0)
		return 0;
	i--;

	fpclose(kmap->stack[i]);

	free(kmap->stack[i]);
	kmap->stack[i] = NULL;

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
static char *include_dirpath0[] = { "", 0 };
static char *include_dirpath1[] = { "", "../include/", "../../include/", 0 };
static char *include_dirpath2[] = { 0, 0, 0, 0 };
static char *include_dirpath3[] = {
	DATADIR "/" KEYMAPDIR "/include/",
	DATADIR "/" KEYMAPDIR "/i386/include/",
	DATADIR "/" KEYMAPDIR "/mac/include/", 0
};

static char *include_suffixes[] = { "", ".inc", 0 };

static int
find_incl_file_near_fn(struct keymap *kmap, char *s, char *fn, lkfile_t *fp)
{
	char *t, *te, *t1, *t2;
	int len, rc = 1;

	if (!fn)
		return 1;

	t = strdup(fn);
	if (t == NULL)
		goto nomem;

	te = strrchr(t, '/');
	if (te) {
		te[1] = 0;
		len = strlen(t);
		include_dirpath2[0] = t;
		include_dirpath2[1] = t1 = malloc(len + 12);
		include_dirpath2[2] = t2 = malloc(len + 15);

		if (t1 == NULL || t2 == NULL)
			goto nomem;

		strcpy(t1, t);
		strcat(t1, "../include/");
		strcpy(t2, t);
		strcat(t2, "../../include/");
		rc = findfile(s, include_dirpath2, include_suffixes, fp);
		free(t1);
		free(t2);
	}
	free(t);
	return rc;

	/* FIXME: free */
nomem:	snprintf(kmap->errmsg, sizeof(kmap->errmsg), _("out of memory"));
	return -1;
}

static int
find_standard_incl_file(struct keymap *kmap, char *s, lkfile_t *fp)
{
	char *pathname;
	int rc = 1;
	int i = 0;

	while (kmap->stack[i]) i++;
	if (i == 0)
		return -1;
	i--;
	pathname = kmap->stack[i]->pathname;

	if (findfile(s, include_dirpath1, include_suffixes, fp)) {
		if ((rc = find_incl_file_near_fn(kmap, s, pathname, fp)) == -1)
			return rc;
	}

	/* If filename is a symlink, also look near its target. */
	if (rc) {
		char buf[MAXPATHLEN], path[MAXPATHLEN], *ptr;
		unsigned int n;

		n = readlink(pathname, buf, sizeof(buf));
		if (n > 0 && n < sizeof(buf)) {
			buf[n] = 0;
			if (buf[0] == '/') {
				rc = find_incl_file_near_fn(kmap, s, buf, fp);

			} else if (strlen(pathname) + n < sizeof(path)) {
				strcpy(path, pathname);
				path[sizeof(path) - 1] = 0;
				ptr = strrchr(path, '/');
				if (ptr)
					ptr[1] = 0;
				strcat(path, buf);
				rc = find_incl_file_near_fn(kmap, s, path, fp);
			}
		}
	}

	if (rc)
		rc = findfile(s, include_dirpath3, include_suffixes, fp);
	return rc;
}

static int
find_incl_file(struct keymap *kmap, char *s, lkfile_t *fp)
{
	char *ev;

	if (!s || !*s)
		return 1;

	if (*s == '/')		/* no path required */
		return (findfile(s, include_dirpath0, include_suffixes, fp));

	if ((ev = getenv("LOADKEYS_INCLUDE_PATH")) != NULL) {
		/* try user-specified path */
		char *user_dir[2] = { 0, 0 };
		while (ev) {
			int rc;
			char *t = strchr(ev, ':');
			char sv = 0;
			if (t) {
				sv = *t;
				*t = 0;
			}
			user_dir[0] = ev;
			if (*ev)
				rc = findfile(s, user_dir, include_suffixes, fp);
			else	/* empty string denotes system path */
				rc = find_standard_incl_file(kmap, s, fp);

			if (rc <= 0)
				return rc;
			if (t)
				*t++ = sv;
			ev = t;
		}
		return 1;
	}
	return find_standard_incl_file(kmap, s, fp);
}

static int
open_include(struct keymap *kmap, char *s, yyscan_t scanner)
{
	int rc;
	lkfile_t fp;

	lkverbose(kmap, 2, _("switching to %s"), s);

	rc = find_incl_file(kmap, s, &fp);
	if (rc > 0) {
		snprintf(kmap->errmsg, sizeof(kmap->errmsg),
			_("cannot open include file %s"), s);
		free(s);
		return -1;
	} else if (rc == -1) {
		free(s);
		return -1;
	}

	free(s);

	return stack_push(kmap, &fp, scanner);
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
Unicode			U\+([0-9a-fA-F]){4}
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
Compose                 compose|Compose|COMPOSE
As			as|As|AS
Usual			usual|Usual|USUAL
For			for|For|FOR
On			on|On|ON
To                      to|To|TO

%%

{Include}		{
				yy_push_state(INCLSTR, yyscanner);
				yyextra->state_ptr++;
			}
<INCLSTR>\"[^\"\n]+\"	{
				char *s = strndup(yytext+1, strlen(yytext)-2);
				if (s == NULL) {
					snprintf(yyextra->errmsg, sizeof(yyextra->errmsg),
						_("out of memory"));
					return(ERROR);
				}

				if (open_include(yyextra, s, yyscanner) == -1)
					return(ERROR);

				yy_pop_state(yyscanner);
				yyextra->state_ptr--;
			}
<INCLSTR>[^"]|\"\"|\"[^"\n]*{Eol}	{
				snprintf(yyextra->errmsg, sizeof(yyextra->errmsg),
					_("expected filename between quotes"));
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

				if (yyextra->state_ptr > 0) {
					yy_pop_state(yyscanner);
					yyextra->state_ptr--;
				}
				return(EOL);
			}
{Blank}+		; /* do nothing */
{Comment}.*/{Eol}	; /* do nothing */
{Equals}		{
				yy_push_state(RVALUE, yyscanner);
				yyextra->state_ptr++;
				yyextra->rvalct = 0;
				return(EQUALS);
			}
{String}		{
				yy_push_state(RVALUE, yyscanner);
				yyextra->state_ptr++;
				return(STRING);
			}
{To}			{
				yy_push_state(RVALUE, yyscanner);
				yyextra->state_ptr++;
				return(TO);
			}
{Unicode}		{
				yylval->num = strtol(yytext + 1, NULL, 16);
				if (yylval->num >= 0xf000) {
					snprintf(yyextra->errmsg, sizeof(yyextra->errmsg),
						_("unicode keysym out of range: %s"),
						yytext);
					return(ERROR);
				}
				return(UNUMBER);
			}
{Decimal}|{Octal}|{Hex}	{
				yylval->num = strtol(yytext, NULL, 0);
				return(NUMBER);
			}
<RVALUE>{Literal}	{	return((yylval->num = ksymtocode(yyextra->prefer_unicode, yytext, TO_AUTO)) == -1 ? ERROR : LITERAL);	}
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
				yylval->num = strtol(yytext + 2, NULL, 8);
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
				yylval->str.data[0] = '\0';
				yylval->str.len = 0;

				yy_push_state(STR, yyscanner);
				yyextra->state_ptr++;
			}
<STR>\\{Octa}		{
				if (yylval->str.len == MAX_PARSER_STRING) {
					snprintf(yyextra->errmsg, sizeof(yyextra->errmsg),
						_("string too long"));
					return(ERROR);
				}
				yylval->str.data[yylval->str.len++] = strtol(yytext + 1, NULL, 8);
			}
<STR>\\\"               {
				if (yylval->str.len == MAX_PARSER_STRING) {
					snprintf(yyextra->errmsg, sizeof(yyextra->errmsg),
						_("string too long"));
					return(ERROR);
				}
				yylval->str.data[yylval->str.len++] = '"';
			}
<STR>\\\\               {
				if (yylval->str.len == MAX_PARSER_STRING) {
					snprintf(yyextra->errmsg, sizeof(yyextra->errmsg),
						_("string too long"));
					return(ERROR);
				}
				yylval->str.data[yylval->str.len++] = '\\';
			}
<STR>\\n		{
				if (yylval->str.len == MAX_PARSER_STRING) {
					snprintf(yyextra->errmsg, sizeof(yyextra->errmsg),
						_("string too long"));
					return(ERROR);
				}
				yylval->str.data[yylval->str.len++] = '\n';
			}
<STR>[^\"\\]*		{
				int len = strlen(yytext);

				if (yylval->str.len + len >= MAX_PARSER_STRING) {
					snprintf(yyextra->errmsg, sizeof(yyextra->errmsg),
						_("string too long"));
					return(ERROR);
				}

				strcpy((char *) yylval->str.data + yylval->str.len, yytext);
				yylval->str.len += len;
			}
<STR>\"			{
				yylval->str.data[yylval->str.len] = '\0';
				yy_pop_state(yyscanner);
				yyextra->state_ptr--;
				return(STRLITERAL);
			}
.			{
				snprintf(yyextra->errmsg, sizeof(yyextra->errmsg),
					_("unknown characters"));
				return(ERROR); /* report any unknown characters */
			}
%%
