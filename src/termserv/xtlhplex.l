%{
/* xtlhplex.l -- lex script for parsing of xtlhp control files

   Copyright 2008 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "config.h"
#include <stdio.h>
#include <ctype.h>
#include "incl_unix.h"
#include "xtlhpdefs.h"
#include "y.tab.h"

int	line_count = 1;
extern	int	errors;

extern struct macro *lookupcreatename(const char *);

static char *brack_cont(char *str)
{
	char	*result, *bp = strchr(str, '('), *ep, save;

	/* Find first non-space char inside bracket */

	if  (!bp)
		return  str;

	do  bp++;
	while  (isspace(*bp));

	/* Find end of bracket */

	ep = strchr(bp, ')');
	if  (!ep)
		return  str;
	do  --ep;
	while  (isspace(*ep));

	/* save char after string */

	save = *++ep;
	*ep = '\0';
	result = stracpy(bp);
	*ep = save;
	return  result;
}

%}

%%

#.*$

[ \t]+
\n	line_count++;

IF	return  IF;
THEN	return  THEN;
ELSE	return  ELSE;
ELIF	return  ELIF;
FI	return  FI;
MSG	return  MSG;
EXIT	return  EXIT;
FLUSH	return	FLUSH;

EQ	{	yylval.intval = STR_COMP + COMP_EQ;	return	STRING_COMP;  }
NE	{	yylval.intval = STR_COMP + COMP_NE;	return	STRING_COMP;  }
LT	{	yylval.intval = STR_COMP + COMP_LT;	return	STRING_COMP;  }
LE	{	yylval.intval = STR_COMP + COMP_LE;	return	STRING_COMP;  }
GT	{	yylval.intval = STR_COMP + COMP_GT;	return	STRING_COMP;  }
GE	{	yylval.intval = STR_COMP + COMP_GE;	return	STRING_COMP;  }

AND	{	return  ANDOP;  }
OR	{	return  OROP;  }

"&&"	{	return  ANDOP;  }
"||"	{	return  OROP;  }

"=="	{	yylval.intval = COMP_EQ;	return	NUMERIC_COMP;  }
"!="	{	yylval.intval = COMP_NE;	return	NUMERIC_COMP;  }
">="	{	yylval.intval = COMP_GE;	return	NUMERIC_COMP;  }
">"	{	yylval.intval = COMP_GT;	return	NUMERIC_COMP;  }
"<="	{	yylval.intval = COMP_LE;	return	NUMERIC_COMP;  }
"<"	{	yylval.intval = COMP_LT;	return	NUMERIC_COMP;  }

":="	return  ONEASSIGN;
"="	return  ASSIGN;

"SNMPVAR"[ \t]*"("[ \t]*[a-zA-Z_0-9\.]+[ \t]*")"	{ yylval.stringval = brack_cont(yytext); return  SNMPVARVAL;  }
"SNMPSTR"[ \t]*"("[ \t]*[a-zA-Z_0-9\.]+[ \t]*")"	{ yylval.stringval = brack_cont(yytext); return  SNMPSTRVAL;  }
"DEFINED"[ \t]*"("[ \t]*[a-zA-Z_0-9\.]+[ \t]*")"	{ yylval.stringval = brack_cont(yytext); return  SNMPDEFINED;  }
"UNDEFINED"[ \t]*"("[ \t]*[a-zA-Z_0-9\.]+[ \t]*")"	{ yylval.stringval = brack_cont(yytext);  return  SNMPUNDEFINED;  }

LASTVAL		return  LASTVAL;
ISNUM		return  ISNUM;
ISSTRING	return	ISSTRING;

BITSET		{	yylval.intval = BIT_SET;	return	BITOP;  }
BITCLEAR	{	yylval.intval = BIT_CLEAR;	return	BITOP;  }
ALLCLEAR  	return	ALLCLEAR;

"("	return  '(';
")"	return  ')';
"!"	return	NOT;

[a-zA-Z_][a-zA-Z0-9_]*  {	yylval.nameval = lookupcreatename(yytext);	return  NAME;  }

-?[0-9]+  {	yylval.longval = atol(yytext);	return  NUMBER;  }

"`".*"`"  {	yytext[yyleng-1] = '\0';	yylval.stringval = stracpy(yytext+1);	yytext[yyleng-1] = '`';  return  BACKQUOTESTR;	}
"'".*"'"  {	yytext[yyleng-1] = '\0';	yylval.stringval = stracpy(yytext+1);	yytext[yyleng-1] = '\'';  return  SQUOTESTR;	}
"\"".*"\""  {	yytext[yyleng-1] = '\0';	yylval.stringval = stracpy(yytext+1);	yytext[yyleng-1] = '\"';  return  DQUOTESTR;	}
"[".*"]"  {	yytext[yyleng-1] = '\0';	yylval.stringval = stracpy(yytext+1);	yytext[yyleng-1] = ']';  return  BRACKETSTR;	}
"{".*"}"  {	yytext[yyleng-1] = '\0';	yylval.stringval = stracpy(yytext+1);	yytext[yyleng-1] = '}';  return  BRACESTR;	}

%%
