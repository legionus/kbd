%option noyywrap
%option nounput
%option noinput
%{
#define YY_NO_INPUT 1

#include <stdlib.h>
#include <linux/kd.h>
#include "ksyms.h"
#include "kbd.h"
#include "xmalloc.h"
#include "findfile.h"
#include "loadkeys.h"

int	rvalct;
struct kbsentry kbs_buf;
char *p, *pmax;

extern int yyerror(const char *s);
extern void stringovfl(void);
extern void lkfatal(const char *fmt, ...);
extern void open_include(char *s);

/* Include file handling - unfortunately flex-specific. */
#define MAX_INCLUDE_DEPTH 20
struct infile {
	FILE *fd;
	int  pipe;
	char *filename;
	int  linenr;
	YY_BUFFER_STATE buffer;

} infile_stack[MAX_INCLUDE_DEPTH];

char *filename = NULL;
int  line_nr = 1;
int  infile_stack_ptr = -1;

void stack_push(FILE *fd, int ispipe, char *filename);
int stack_pop(void);

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

{Include}               {BEGIN(INCLSTR);}
<INCLSTR>\"[^\"\n]+\"	{char *s = xstrndup(yytext+1, strlen(yytext)-2);
			  /* use static pathname to store *s ? */
			 open_include(s);
			 BEGIN(0);
			}
<INCLSTR>[^"]|\"\"|\"[^"\n]*{Eol}	{
			  yyerror("expected filename between quotes");
			  BEGIN(0); }
<<EOF>>			{stack_pop();
			 if (!YY_CURRENT_BUFFER) yyterminate();
			}
{Continuation}		{line_nr++;}
{Eol}			{line_nr++;BEGIN(0);return(EOL);}
{Blank}+		; /* do nothing */
{Comment}.*/{Eol}	; /* do nothing */
{Equals}		{BEGIN(RVALUE);rvalct=0;return(EQUALS);}
\-			{return(DASH);}
\,			{return(COMMA);}
\+			{return(PLUS);}
{Unicode}		{yylval=strtol(yytext+1,NULL,16);if(yylval>=0xf000)lkfatal("unicode keysym out of range: %s",yytext);return(UNUMBER);}
{Decimal}|{Octal}|{Hex}	{yylval=strtol(yytext,NULL,0);return(NUMBER);}
<RVALUE>{Literal}	{return((yylval=ksymtocode(yytext, TO_AUTO))==-1?ERROR:LITERAL);}
{Charset}		{return(CHARSET);}
{Keymaps}		{return(KEYMAPS);}
{Keycode}		{return(KEYCODE);}
{String}		{BEGIN(RVALUE);return(STRING);}
{Plain}			{return(PLAIN);}
{Shift}			{return(SHIFT);}
{Control}		{return(CONTROL);}
{Alt}			{return(ALT);}
{AltGr}			{return(ALTGR);}
{ShiftL}		{return(SHIFTL);}
{ShiftR}		{return(SHIFTR);}
{CtrlL}			{return(CTRLL);}
{CtrlR}			{return(CTRLR);}
{CapsShift}		{return(CAPSSHIFT);}
{AltIsMeta}		{return(ALT_IS_META);}
{Strings}		{return(STRINGS);}
{Compose}               {return(COMPOSE);}
{As}			{return(AS);}
{Usual}			{return(USUAL);}
{To}                    {BEGIN(RVALUE); return(TO);}
{On}			{return(ON);}
{For}			{return(FOR);}
'\\{Octa}'              {yylval = strtol(yytext+2,NULL,8); return(CCHAR);}
'\\.'                   {yylval = (unsigned char) yytext[2]; return(CCHAR);}
'.'                     {yylval = (unsigned char) yytext[1]; return(CCHAR);}
\"			{p=(char *) kbs_buf.kb_string;
				pmax=p+sizeof(kbs_buf.kb_string)-1;
				BEGIN(STR);}
<STR>\\{Octa}		{if(p>=pmax)stringovfl();*p++=strtol(yytext+1,NULL,8);}
<STR>\\\"               {if(p>=pmax)stringovfl();*p++='"';}
<STR>\\\\               {if(p>=pmax)stringovfl();*p++='\\';}
<STR>\\n		{if(p>=pmax)stringovfl();*p++='\n';}
<STR>[^\"\\]*		{char *ptmp=p;p+=strlen(yytext); 
				if(p>pmax)stringovfl();strcpy(ptmp,yytext);}
<STR>\"			{*p='\0';BEGIN(0);return(STRLITERAL);}
.			{return(ERROR); /* report any unknown characters */}

%%

#include "ksyms.h"
#include <linux/keyboard.h>

void
stringovfl(void) {
	lkfatal("string too long");
}

void
stack_push(FILE *fd, int is_pipe, char *fname) {
	if (infile_stack_ptr >= MAX_INCLUDE_DEPTH)
		lkfatal("includes are nested too deeply");

	infile_stack_ptr++;

	printf("Stack push %s\n", fname);

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

int
stack_pop(void) {
	fpclose1(infile_stack[infile_stack_ptr].fd, infile_stack[infile_stack_ptr].pipe);
	printf("Stack pop %s\n", infile_stack[infile_stack_ptr].filename);

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

		printf("Stack change %s\n", infile_stack[infile_stack_ptr].filename);

		// Switch to new buffer
		yy_switch_to_buffer(infile_stack[infile_stack_ptr].buffer);
		return 0;
	}

	return 1;
}

