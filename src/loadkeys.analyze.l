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
#include "xmalloc.h"
#include "findfile.h"
#include "loadkeys.h"

int rvalct;
struct kbsentry kbs_buf;
static char *p, *pmax;

extern int verbose;
extern int yyerror(const char *s);
extern void lkfatal(const char *fmt, ...);

void stack_push(FILE *fd, int is_pipe, char *fname);

/* Include file handling - unfortunately flex-specific. */
#define MAX_INCLUDE_DEPTH 20
static struct infile {
	FILE *fd;
	char *filename;
	int  pipe;
	int  linenr;
	YY_BUFFER_STATE buffer;
} infile_stack[MAX_INCLUDE_DEPTH];

char *filename = NULL;
int  line_nr = 1;

static int infile_stack_ptr = -1;
static int state_ptr = 0;

void stack_push(FILE *fd, int is_pipe, char *fname) {
	if (infile_stack_ptr >= MAX_INCLUDE_DEPTH)
		lkfatal(_("includes are nested too deeply"));

	infile_stack_ptr++;

	infile_stack[infile_stack_ptr].buffer   = yy_create_buffer(fd, YY_BUF_SIZE);
	infile_stack[infile_stack_ptr].fd       = fd;
	infile_stack[infile_stack_ptr].pipe     = is_pipe;
	infile_stack[infile_stack_ptr].filename = strdup(fname);
	infile_stack[infile_stack_ptr].linenr   = line_nr;

	yy_switch_to_buffer(infile_stack[infile_stack_ptr].buffer);

	// Set global variables
	filename = infile_stack[infile_stack_ptr].filename;
	line_nr  = 1;
}

static int stack_pop(void) {
	fpclose1(infile_stack[infile_stack_ptr].fd, infile_stack[infile_stack_ptr].pipe);

	// Free previous filename
	xfree(infile_stack[infile_stack_ptr].filename);

	// Destroy previous buffer
	yy_delete_buffer(infile_stack[infile_stack_ptr].buffer);

	infile_stack_ptr--;

	if (infile_stack_ptr >= 0) {
		// Set new state
		//yyin     = infile_stack[infile_stack_ptr].fd; // ???
		ispipe   = infile_stack[infile_stack_ptr].pipe;
		filename = infile_stack[infile_stack_ptr].filename;
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

static FILE *find_incl_file_near_fn(char *s, char *fn)
{
	FILE *f = NULL;
	char *t, *te, *t1, *t2;
	int len;

	if (!fn)
		return NULL;

	t = xstrdup(fn);
	te = strrchr(t, '/');
	if (te) {
		te[1] = 0;
		len = strlen(t);
		include_dirpath2[0] = t;
		include_dirpath2[1] = t1 = xmalloc(len + 12);
		include_dirpath2[2] = t2 = xmalloc(len + 15);
		strcpy(t1, t);
		strcat(t1, "../include/");
		strcpy(t2, t);
		strcat(t2, "../../include/");
		f = findfile(s, include_dirpath2, include_suffixes);
		xfree(t1);
		xfree(t2);
	}
	xfree(t);
	return f;
}

static FILE *find_standard_incl_file(char *s)
{
	FILE *f;

	f = findfile(s, include_dirpath1, include_suffixes);
	if (!f)
		f = find_incl_file_near_fn(s, filename);

	/* If filename is a symlink, also look near its target. */
	if (!f) {
		char buf[MAXPATHLEN], path[MAXPATHLEN], *ptr;
		unsigned int n;

		n = readlink(filename, buf, sizeof(buf));
		if (n > 0 && n < sizeof(buf)) {
			buf[n] = 0;
			if (buf[0] == '/') {
				f = find_incl_file_near_fn(s, buf);

			} else if (strlen(filename) + n < sizeof(path)) {
				strcpy(path, filename);
				path[sizeof(path) - 1] = 0;
				ptr = strrchr(path, '/');
				if (ptr)
					ptr[1] = 0;
				strcat(path, buf);
				f = find_incl_file_near_fn(s, path);
			}
		}
	}

	if (!f)
		f = findfile(s, include_dirpath3, include_suffixes);
	return f;
}

static FILE *find_incl_file(char *s)
{
	FILE *f;
	char *ev;

	if (!s || !*s)
		return NULL;

	if (*s == '/')		/* no path required */
		return (findfile(s, include_dirpath0, include_suffixes));

	if ((ev = getenv("LOADKEYS_INCLUDE_PATH")) != NULL) {
		/* try user-specified path */
		char *user_dir[2] = { 0, 0 };
		while (ev) {
			char *t = strchr(ev, ':');
			char sv = 0;
			if (t) {
				sv = *t;
				*t = 0;
			}
			user_dir[0] = ev;
			if (*ev)
				f = findfile(s, user_dir, include_suffixes);
			else	/* empty string denotes system path */
				f = find_standard_incl_file(s);
			if (f)
				return f;
			if (t)
				*t++ = sv;
			ev = t;
		}
		return NULL;
	}
	return find_standard_incl_file(s);
}

static void open_include(char *s)
{
	FILE *fd;

	if (verbose)
		fprintf(stdout, _("switching to %s\n"), s);

	fd = find_incl_file(s);
	if (!fd)
		lkfatal(_("cannot open include file %s"), s);

	xfree(s);

	stack_push(fd, ispipe, pathname);
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
				char *s = xstrndup(yytext+1, strlen(yytext)-2);
				/* use static pathname to store *s ? */
				open_include(s);
				yy_pop_state();
				state_ptr--;
			}
<INCLSTR>[^"]|\"\"|\"[^"\n]*{Eol}	{
				yyerror(_("expected filename between quotes"));
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
				rvalct = 0;
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
				if (yylval >= 0xf000)
					lkfatal(_("unicode keysym out of range: %s"), yytext);
				return(UNUMBER);
			}
{Decimal}|{Octal}|{Hex}	{
				yylval = strtol(yytext, NULL, 0);
				return(NUMBER);
			}
<RVALUE>{Literal}	{	return((yylval = ksymtocode(yytext, TO_AUTO)) == -1 ? ERROR : LITERAL);	}
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
				p = (char *) kbs_buf.kb_string;
				pmax = p + sizeof(kbs_buf.kb_string) - 1;
				yy_push_state(STR);
				state_ptr++;
			}
<STR>\\{Octa}		{
				if (p >= pmax)
					lkfatal(_("string too long"));
				*p++ = strtol(yytext + 1, NULL, 8);
			}
<STR>\\\"               {
				if (p >= pmax)
					lkfatal(_("string too long"));
				*p++ = '"';
			}
<STR>\\\\               {
				if (p >= pmax)
					lkfatal(_("string too long"));
				*p++ = '\\';
			}
<STR>\\n		{
				if (p >= pmax)
					lkfatal(_("string too long"));
				*p++ = '\n';
			}
<STR>[^\"\\]*		{
				char *ptmp = p;
				p += strlen(yytext);
				if (p > pmax)
					lkfatal(_("string too long"));
				strcpy(ptmp, yytext);
			}
<STR>\"			{
				*p = '\0';
				yy_pop_state();
				state_ptr--;
				return(STRLITERAL);
			}
.			{
				return(ERROR); /* report any unknown characters */
			}
%%
