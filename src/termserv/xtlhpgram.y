/* xtlhpgram.y -- bison/yacc grammar for xtlhp control file

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

%{
#include "config.h"
#include <stdio.h>
#include "incl_unix.h"
#include "xtlhpdefs.h"

extern int  yylex();

int	errors = 0;

extern	int	line_count;

extern	struct	command	*Control_list;

void  yyerror(char *msg)
{
	fprintf(stderr, "Parse error: %s on line %d\n", msg, line_count);
	errors++;
}

struct	compare	*alloc_compare(int type)
{
	struct	compare	*result;
	if  (!(result = (struct compare *) malloc(sizeof(struct compare))))
		nomem();
	result->type = type;
	return  result;
}

struct value *alloc_value(int type)
{
	struct	value	*result;
	if  (!(result = (struct value *) malloc(sizeof(struct value))))
		nomem();
	result->type = type;
	return  result;
}

struct boolexpr *alloc_boolexpr(int type)
{
	struct  boolexpr  *result;
	if  (!(result = (struct boolexpr *) malloc(sizeof(struct boolexpr))))
		nomem();
	result->type = type;
	return  result;
}

struct	command	*alloc_cmd(int type)
{
	struct	command	*result;
	if  (!(result = (struct command *) malloc(sizeof(struct command))))
		nomem();
	result->type = type;
	result->next = (struct command *) 0;
	return  result;
}

%}

%union {
	int		intval;
	long		longval;
	char		*stringval;
	struct	value	*valval;
	struct	macro	*nameval;
	struct	boolexpr *exprval;
	struct	compare	*cmpval;
	struct	command	*cmdval;
};

%token	<longval>	NUMBER
%token	<stringval>	BACKQUOTESTR SQUOTESTR DQUOTESTR BRACKETSTR BRACESTR SNMPVARVAL SNMPSTRVAL SNMPDEFINED SNMPUNDEFINED
%token	<nameval>	NAME
%token	<intval>	NUMERIC_COMP STRING_COMP BITOP
%token			ASSIGN ONEASSIGN IF THEN ELSE ELIF FI MSG EXIT FLUSH LASTVAL ISNUM ISSTRING NOT ALLCLEAR

%left	OROP
%left	ANDOP
%right	NOT

%type	<cmdval>	ctrllist cmd ass_cmd if_cmd msg_cmd exit_cmd flush_cmd
%type	<cmdval>	elsepart eliflist
%type	<exprval>	ifexpr condexpr compexpr bitexpr defexpr typeexpr
%type	<valval>	value stringvalue longvalue
%type	<intval>	compare
%type	<stringval>	bitstring

%start	ctrlprog

%%

ctrlprog:	ctrllist
		{
			Control_list = $1;
		}
		;

ctrllist:	cmd
		|
		ctrllist cmd
		{
			struct	command	*cp;
			for  (cp = $1;  cp->next;  cp = cp->next)
				;
			cp->next = $2;
			$$ = $1;
		}
		;

cmd:		ass_cmd | if_cmd | msg_cmd | exit_cmd | flush_cmd ;

ass_cmd:	NAME ASSIGN value
		{
			$$ = alloc_cmd(CMD_ASS);
			$$->cmd_un.ass.ass_name = $1;
			$$->cmd_un.ass.ass_value = $3;
		}
		|
		NAME ONEASSIGN value
		{
			$$ = alloc_cmd(CMD_ONEASS);
			$$->cmd_un.ass.ass_name = $1;
			$$->cmd_un.ass.ass_value = $3;
		}
		;

if_cmd:		IF ifexpr THEN ctrllist elsepart FI
		{
			$$ = alloc_cmd(CMD_IF);
			$$->cmd_un.ifthen.comp_expr = $2;
			$$->cmd_un.ifthen.thenpart = $4;
			$$->cmd_un.ifthen.elsepart = $5;
		}
		;

elsepart:	/* empty */
		{
			$$ = (struct command *) 0;
		}
		|
		ELSE ctrllist
		{
			$$ = $2;
		}
		|
		eliflist
		|
		eliflist ELSE ctrllist
		{
			struct  command	 *cp;
			for  (cp = $1;  cp->cmd_un.ifthen.elsepart;  cp = cp->cmd_un.ifthen.elsepart)
				;
			cp->cmd_un.ifthen.elsepart = $3;
		}
		;

eliflist:	ELIF ifexpr THEN ctrllist
		{
			$$ = alloc_cmd(CMD_IF);
			$$->cmd_un.ifthen.comp_expr = $2;
			$$->cmd_un.ifthen.thenpart = $4;
			$$->cmd_un.ifthen.elsepart = (struct command *) 0;
		}
		|
		eliflist ELIF ifexpr THEN ctrllist
		{
			struct  command	 *cp, *ep;
			for  (cp = $1;  cp->cmd_un.ifthen.elsepart;  cp = cp->cmd_un.ifthen.elsepart)
				;
			cp->cmd_un.ifthen.elsepart = ep = alloc_cmd(CMD_IF);
			ep->cmd_un.ifthen.comp_expr = $3;
			ep->cmd_un.ifthen.thenpart = $5;
			ep->cmd_un.ifthen.elsepart = (struct command *) 0;
		}
		;

msg_cmd:	MSG stringvalue
		{
			$$ = alloc_cmd(CMD_MSG);
			$$->cmd_un.msgval = $2;
		}
		;

exit_cmd:	EXIT longvalue
		{
			$$ = alloc_cmd(CMD_EXIT);
			$$->cmd_un.exitcode = $2;
		}
		;

flush_cmd:	FLUSH
		{
			$$ = alloc_cmd(CMD_FLUSH);
		}
		;

ifexpr:		condexpr
		|
		NOT ifexpr
			{
				$$ = alloc_boolexpr(NOTEXPR);
				$$->left_un.expr = 0;
				$$->rightexpr = $2;
			}
		|
		'(' ifexpr ')'
			{
				$$ = $2;
			}
		|
		ifexpr ANDOP ifexpr
			{
				$$ = alloc_boolexpr(ANDEXPR);
				$$->left_un.expr = $1;
				$$->rightexpr = $3;
			}
		|
		ifexpr OROP ifexpr
			{
				$$ = alloc_boolexpr(OREXPR);
				$$->left_un.expr = $1;
				$$->rightexpr = $3;
			}
		;

condexpr:	compexpr | bitexpr | defexpr | typeexpr;

compexpr:	value compare value
		{
			struct  compare  *cmp = alloc_compare($2);
			cmp->left = $1;
			cmp->right = $3;
			$$ = alloc_boolexpr(COMPARE);
			$$->left_un.comp = cmp;
			$$->rightexpr = 0;
		}
		;

bitexpr:	bitstring BITOP value
		{
			$$ = alloc_boolexpr(BITOPER);
			$$->left_un.snmpstring = $1;
			$$->rightexpr = alloc_boolexpr(ISNUMVAL);
			$$->rightexpr->left_un.val = $3;
			$$->rightexpr->rightexpr = 0;
			if  ($2 == BIT_CLEAR)  {
				struct boolexpr *nr = alloc_boolexpr(NOTEXPR);
				nr->left_un.expr = 0;
				nr->rightexpr = $$;
				$$ = nr;
			}
		}
		|
		ALLCLEAR bitstring
		{
			$$ = alloc_boolexpr(ALL_CLEAR);
			$$->left_un.snmpstring = $2;
			$$->rightexpr = 0;
		}
		;

bitstring:	SNMPSTRVAL  |  	LASTVAL { $$ = 0;  } ;

defexpr:	SNMPDEFINED
			{
				$$ = alloc_boolexpr(VARDEFINED);
				$$->left_un.snmpstring = $1;
				$$->rightexpr = 0;
			}
		|
		SNMPUNDEFINED
			{
				$$ = alloc_boolexpr(VARUNDEFINED);
				$$->left_un.snmpstring = $1;
				$$->rightexpr = 0;
			}
		;

typeexpr:	ISNUM value
			{
				$$ = alloc_boolexpr(ISNUMVAL);
				$$->left_un.val = $2;
				$$->rightexpr = 0;
			}
		|
		ISSTRING value
			{
				$$ = alloc_boolexpr(ISSTRINGVAL);
				$$->left_un.val = $2;
				$$->rightexpr = 0;
			}
		;

value:		NAME
		{
			$$ = alloc_value(NAME_VALUE);
			$$->val_un.namev = $1;
		}
		|
		stringvalue
		|
		longvalue
		|
		LASTVAL
		{
			$$ = alloc_value(LASTVAL_VALUE);
			$$->val_un.longval = 0;
		}
		;

stringvalue:	BACKQUOTESTR
		{
			$$ = alloc_value(CMD_STRING_VALUE);
			$$->val_un.stringval = $1;
		}
		|
		SQUOTESTR
		{
			$$ = alloc_value(FSTRING_VALUE);
			$$->val_un.stringval = $1;
		}
		|
		DQUOTESTR
		{
			$$ = alloc_value(STRING_VALUE);
			$$->val_un.stringval = $1;
		}
		|
		BRACESTR
		{

			$$ = alloc_value(BRACE_VALUE);
			$$->val_un.stringval = $1;
		}
		|
		SNMPSTRVAL
		{
			$$ = alloc_value(SNMPVAR_VALUE);
			$$->val_un.stringval = $1;
		}
		;

longvalue:	NUMBER
		{
			$$ = alloc_value(NUM_VALUE);
			$$->val_un.longval = $1;
		}
		|
		BRACKETSTR
		{

			$$ = alloc_value(CMD_NUM_VALUE);
			$$->val_un.stringval = $1;
		}
		|
		SNMPVARVAL
		{
			$$ = alloc_value(SNMPVAR_VALUE);
			$$->val_un.stringval = $1;
		}
		;

compare:	NUMERIC_COMP | STRING_COMP ;
