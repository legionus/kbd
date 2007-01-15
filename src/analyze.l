%{
#include <stdlib.h>
#include <linux/kd.h>
#include "ksyms.h"
#include "xmalloc.h"

extern int line_nr;
int	yylval;
int	rvalct;
struct kbsentry kbs_buf;
char *p, *pmax;
char *filename;

#undef yywrap
extern int yywrap(void);
extern int yyerror(const char *s);
extern void stringovfl(void);
extern void lkfatal(const char *s);
extern void lkfatal1(const char *s, const char *s2);
extern void open_include(char *s);
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
AltIsMeta		[aA][lL][tT][-_][iI][sS][-_][mM][eE][tT][aA]
Strings			strings|Strings|STRINGS
Compose                 compose|Compose|COMPOSE
As			as|As|AS
Usual			usual|Usual|USUAL
For			for|For|FOR
On			on|On|ON
To                      to|To|TO
%%
{Include}               {BEGIN(INCLSTR);}
<INCLSTR>\"[^"\n]+\"	{ int l; char *s;
			 l = strlen(yytext);
			 s = xmalloc(l);
			 strcpy(s, yytext+1);
			 s[l-2] = 0;		/* wipe out " */
			 open_include(s);
			 BEGIN(0);
			}
<INCLSTR>[^"]|\"\"|\"[^"\n]*{Eol}	{
			  yyerror("expected filename between quotes");
			  BEGIN(0); }
{Continuation}		{line_nr++;}
{Eol}			{line_nr++;BEGIN(0);return(EOL);}
{Blank}+		; /* do nothing */
{Comment}.*/{Eol}	; /* do nothing */
{Equals}		{BEGIN(RVALUE);rvalct=0;return(EQUALS);}
\-			{return(DASH);}
\,			{return(COMMA);}
\+			{return(PLUS);}
{Unicode}		{yylval=strtol(yytext+1,NULL,16);return(UNUMBER);}
{Decimal}|{Octal}|{Hex}	{yylval=strtol(yytext,NULL,0);return(NUMBER);}
<RVALUE>{Literal}	{return((yylval=ksymtocode(yytext))==-1?ERROR:LITERAL);}
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
{AltIsMeta}		{return(ALT_IS_META);}
{Strings}		{return(STRINGS);}
{Compose}               {return(COMPOSE);}
{As}			{return(AS);}
{Usual}			{return(USUAL);}
{To}                    {BEGIN(RVALUE); return(TO);}
{On}			{return(ON);}
{For}			{return(FOR);}
'\\{Octa}'              {yylval = strtol(yytext+2,NULL,8); return(CCHAR);}
'\\.'                   {yylval = yytext[2]; return(CCHAR);}
'.'                     {yylval = yytext[1]; return(CCHAR);}
\"			{p=kbs_buf.kb_string;
				pmax=p+sizeof(kbs_buf.kb_string)-1;
				BEGIN(STR);}
<STR>\\{Octa}		{if(p>=pmax)stringovfl();*p++=strtol(yytext+1,NULL,8);}
<STR>\\\"               {if(p>=pmax)stringovfl();*p++='"';}
<STR>\\\\               {if(p>=pmax)stringovfl();*p++='\\';}
<STR>\\n		{if(p>=pmax)stringovfl();*p++='\n';}
<STR>[^"\\]*		{char *ptmp=p;p+=strlen(yytext);
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
