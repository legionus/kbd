%option stack
%option noyy_top_state
%option noyywrap
%option nounput
%option noinput
%{
#define YY_NO_INPUT 1

#include <stdlib.h>
#include <unistd.h> /* readlink */
#include <sys/param.h>
#include <linux/kd.h>
#include <linux/keyboard.h>

#include "ksyms.h"
#include "nls.h"
#include "kbd.h"
#include "paths.h"
#include "findfile.h"
#include "loadkeys.h"

#include "loadkeys.keymap.h"

extern struct keymap kmap;
extern int lkverbose(int level, const char *fmt, ...);

int stack_push(lkfile_t *fp);

/* Include file handling - unfortunately flex-specific. */
#define MAX_INCLUDE_DEPTH 20
static struct infile {
	lkfile_t fp;
	int  linenr;
	YY_BUFFER_STATE buffer;
} infile_stack[MAX_INCLUDE_DEPTH];

static char *filename = NULL;
static int  line_nr = 1;

static char *p, *pmax;

static int infile_stack_ptr = -1;
static int state_ptr = 0;

int
stack_push(lkfile_t *fp)
{
	if (infile_stack_ptr >= MAX_INCLUDE_DEPTH) {
		snprintf(kmap.errmsg, sizeof(kmap.errmsg),
			_("includes are nested too deeply"));
		return -1;
	}

	infile_stack_ptr++;

	infile_stack[infile_stack_ptr].buffer = yy_create_buffer(fp->fd, YY_BUF_SIZE);
	infile_stack[infile_stack_ptr].linenr = line_nr;

	memcpy(&infile_stack[infile_stack_ptr].fp, fp, sizeof(fp));
	yy_switch_to_buffer(infile_stack[infile_stack_ptr].buffer);

	// Set global variables
	filename = infile_stack[infile_stack_ptr].fp.pathname;
	line_nr  = 1;

	return 0;
}

static int
stack_pop(void) {
	fpclose(&infile_stack[infile_stack_ptr].fp);

	// Destroy previous buffer
	yy_delete_buffer(infile_stack[infile_stack_ptr].buffer);

	infile_stack_ptr--;

	if (infile_stack_ptr >= 0) {
		// Set new state
		filename = infile_stack[infile_stack_ptr].fp.pathname;
		line_nr  = infile_stack[infile_stack_ptr].linenr;

		// Switch to new buffer
		yy_switch_to_buffer(infile_stack[infile_stack_ptr].buffer);
		return 0;
	}

	return 1;
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
find_incl_file_near_fn(char *s, char *fn, lkfile_t *fp)
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
nomem:	snprintf(kmap.errmsg, sizeof(kmap.errmsg), _("out of memory"));
	return -1;
}

static int
find_standard_incl_file(char *s, lkfile_t *fp)
{
	int rc = 1;

	if (findfile(s, include_dirpath1, include_suffixes, fp)) {
		if ((rc = find_incl_file_near_fn(s, filename, fp)) == -1)
			return rc;
	}

	/* If filename is a symlink, also look near its target. */
	if (rc) {
		char buf[MAXPATHLEN], path[MAXPATHLEN], *ptr;
		unsigned int n;

		n = readlink(filename, buf, sizeof(buf));
		if (n > 0 && n < sizeof(buf)) {
			buf[n] = 0;
			if (buf[0] == '/') {
				rc = find_incl_file_near_fn(s, buf, fp);

			} else if (strlen(filename) + n < sizeof(path)) {
				strcpy(path, filename);
				path[sizeof(path) - 1] = 0;
				ptr = strrchr(path, '/');
				if (ptr)
					ptr[1] = 0;
				strcat(path, buf);
				rc = find_incl_file_near_fn(s, path, fp);
			}
		}
	}

	if (rc)
		rc = findfile(s, include_dirpath3, include_suffixes, fp);
	return rc;
}

static int
find_incl_file(char *s, lkfile_t *fp)
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
				rc = find_standard_incl_file(s, fp);

			if (rc <= 0)
				return rc;
			if (t)
				*t++ = sv;
			ev = t;
		}
		return 1;
	}
	return find_standard_incl_file(s, fp);
}

static int
open_include(char *s)
{
	int rc;
	lkfile_t fp;

	lkverbose(2, _("switching to %s"), s);

	rc = find_incl_file(s, &fp);
	if (rc > 0) {
		snprintf(kmap.errmsg, sizeof(kmap.errmsg),
			_("cannot open include file %s"), s);
		free(s);
		return -1;
	} else if (rc == -1) {
		free(s);
		return -1;
	}

	free(s);

	return stack_push(&fp);
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
%{
/* To protect from wrong code in the higher level parser (loadkeys.y), 
 * make sure we do not return a sensible value if we
 * do not intend to. (So wrong code would crash, or issue an error message.
 * I mean the code like the one for PLAIN handling in past loadkeys.y.
 * What to choose as the "bad" value: 0, -1?.. 
 * Well, make it equal to CODE_FOR_UNKNOWN_KSYM as the most efficient thing.
 */
#define YYLVAL_UNDEF CODE_FOR_UNKNOWN_KSYM
/* Every time yylex is called, initialize the associated
 * yylval to YYLVAL_UNDEF. Then it might be overwritten by specific rules. */
  yylval = YYLVAL_UNDEF;
%}

{Include}		{
				yy_push_state(INCLSTR);
				state_ptr++;
			}
<INCLSTR>\"[^\"\n]+\"	{
				char *s = strndup(yytext+1, strlen(yytext)-2);
				if (s == NULL) {
					snprintf(kmap.errmsg, sizeof(kmap.errmsg),
						_("out of memory"));
					return(ERROR);
				}

				/* use static pathname to store *s ? */
				if (open_include(s) == -1)
					return(ERROR);
				yy_pop_state();
				state_ptr--;
			}
<INCLSTR>[^"]|\"\"|\"[^"\n]*{Eol}	{
				snprintf(kmap.errmsg, sizeof(kmap.errmsg),
					_("expected filename between quotes"));
				return(ERROR);
			}
<<EOF>>			{
				stack_pop();
				if (!YY_CURRENT_BUFFER)
					yyterminate();
			}
{Continuation}		{
				line_nr++;
			}
{Eol}			{
				line_nr++;
				if (state_ptr > 0) {
					yy_pop_state();
					state_ptr--;
				}
				return(EOL);
			}
{Blank}+		; /* do nothing */
{Comment}.*/{Eol}	; /* do nothing */
{Equals}		{
				yy_push_state(RVALUE);
				state_ptr++;
				kmap.rvalct = 0;
				return(EQUALS);
			}
{String}		{
				yy_push_state(RVALUE);
				state_ptr++;
				return(STRING);
			}
{To}			{
				yy_push_state(RVALUE);
				state_ptr++;
				return(TO);
			}
{Unicode}		{
				yylval = strtol(yytext + 1, NULL, 16);
				if (yylval >= 0xf000) {
					snprintf(kmap.errmsg, sizeof(kmap.errmsg),
						_("unicode keysym out of range: %s"),
						yytext);
					return(ERROR);
				}
				return(UNUMBER);
			}
{Decimal}|{Octal}|{Hex}	{
				yylval = strtol(yytext, NULL, 0);
				return(NUMBER);
			}
<RVALUE>{Literal}	{	return((yylval = ksymtocode(kmap.prefer_unicode, yytext, TO_AUTO)) == -1 ? ERROR : LITERAL);	}
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
				yylval = strtol(yytext + 2, NULL, 8);
				return(CCHAR);
			}
'\\.'                   {
				yylval = (unsigned char) yytext[2];
				return(CCHAR);
			}
'.'                     {
				yylval = (unsigned char) yytext[1];
				return(CCHAR);
			}
\"			{
				p = (char *) kmap.string;
				pmax = p + sizeof(kmap.string) - 1;
				yy_push_state(STR);
				state_ptr++;
			}
<STR>\\{Octa}		{
				if (p >= pmax) {
					snprintf(kmap.errmsg, sizeof(kmap.errmsg),
						_("string too long"));
					return(ERROR);
				}
				*p++ = strtol(yytext + 1, NULL, 8);
			}
<STR>\\\"               {
				if (p >= pmax) {
					snprintf(kmap.errmsg, sizeof(kmap.errmsg),
						_("string too long"));
					return(ERROR);
				}
				*p++ = '"';
			}
<STR>\\\\               {
				if (p >= pmax) {
					snprintf(kmap.errmsg, sizeof(kmap.errmsg),
						_("string too long"));
					return(ERROR);
				}
				*p++ = '\\';
			}
<STR>\\n		{
				if (p >= pmax) {
					snprintf(kmap.errmsg, sizeof(kmap.errmsg),
						_("string too long"));
					return(ERROR);
				}
				*p++ = '\n';
			}
<STR>[^\"\\]*		{
				char *ptmp = p;
				p += strlen(yytext);
				if (p > pmax) {
					snprintf(kmap.errmsg, sizeof(kmap.errmsg),
						_("string too long"));
					return(ERROR);
				}
				strcpy(ptmp, yytext);
			}
<STR>\"			{
				*p = '\0';
				yy_pop_state();
				state_ptr--;
				return(STRLITERAL);
			}
.			{
				snprintf(kmap.errmsg, sizeof(kmap.errmsg),
					_("unknown characters"));
				return(ERROR); /* report any unknown characters */
			}
%%
