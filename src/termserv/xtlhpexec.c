/* xtlhpexec.c -- execute script for xtlhp

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
#include <sys/types.h>
#include <stdio.h>
#include <ctype.h>
#ifdef	OS_FREEBSD
#include <sys/time.h>
#endif
#include <math.h>
#include "xtlhpdefs.h"
#include "incl_unix.h"
#include "incl_net.h"
#include "defaults.h"
#include "asn.h"
#include "snmp.h"

extern	int	debug;
extern	int	snmp_next;
extern	char	*community;

static	struct	snmp_result	last_result;
static	int	snmp_ok = RES_UNDEF;

extern void	do_flush(void);
extern char	*expand(const char *);
extern void	snmp_xmit(char *, int);
extern unsigned	snmp_recv(char *, int);
extern int	snmp_wait(void);

/* Get hex digit.  This is for IBM who in their INFINITE and SUBLIME
   WISDOM choose to return strings as hex like 4a:43:50 */

static int	ghexdig(const int ch)
{
	switch  (ch)  {
	default:
		return  -1;
	case  '0':case  '1':case  '2':case  '3':case  '4':
	case  '5':case  '6':case  '7':case  '8':case  '9':
		return  ch - '0';
	case  'a':case  'b':case  'c':case  'd':case  'e':case  'f':
		return  ch - 'a' + 10;
	case  'A':case  'B':case  'C':case  'D':case  'E':case  'F':
		return  ch - 'A' + 10;
	}
}

/* Go through the motions of extracting a string by running a command.
   We look for a string of the form Value:[ \t]*string We don't
   bother to look for some regular expression as we can just put
   sed or something into the command to rejig it if necessary
   (e.g. IBM).  */

char  *execute_str(char *orig)
{
	char	*expstr = expand(orig), *cp;
	FILE	*pf;
	char	inbuf[120];

	if  (!(pf = popen(expstr, "r")))  {
		if  (debug > 0)
			fprintf(stderr, "Popen failed for %s (orig %s)\n", expstr, orig);
		free(expstr);
		return  stracpy("");
	}
	free(expstr);

	/* Just search for a line containing the string.  */

	while  (fgets(inbuf, sizeof(inbuf), pf))  {
		int	lng = strlen(inbuf) - 1;

		if  (inbuf[lng] == '\n')
			inbuf[lng] = '\0';

		if  (ncstrncmp(inbuf, "value:", 6) != 0)
			continue;

		/* Got it, skip leading spaces/tabs */

		cp = &inbuf[6];
		while  (isspace(*cp))
			cp++;
		pclose(pf);

		if  (debug > 1)
			fprintf(stderr, "Returning value string %s from %s\n", cp, orig);
		return  stracpy(cp);
	}
	pclose(pf);

	if  (debug > 0)
		fprintf(stderr, "Failed to find value string %s\n", orig);
	return  stracpy("");
}

int	eval_snmp(char *str)
{
	char		*expanded;
	asn_octet	*coded;
	unsigned	outlen, inlen;
	asn_octet	inbuf[200];

	/* Free anything we have left over from last time */

	if  (snmp_ok == RES_OK  &&  last_result.res_type == RES_TYPE_STRING)  {
		if  (last_result.res_id_string)  {
			free(last_result.res_id_string);
			last_result.res_id_string = (char *) 0;
		}
		free(last_result.res_un.res_string);
		snmp_ok = RES_UNDEF;
	}

	if  (debug > 1)
		fprintf(stderr, "Evaluating SNMP var %s\n", str);

	expanded = expand(str);

	if  (debug > 1  &&  strcmp(str, expanded) != 0)
		fprintf(stderr, "SNMP var is %s\n", expanded);

	coded = gen_snmp_get(community, expanded, &outlen, snmp_next);
	free(expanded);

	if  (debug > 3)  {
		fprintf(stderr, "Outgoing SNMP request...\n");
		prinbuf(coded, outlen);
	}

	snmp_xmit((char *) coded, (int) outlen);
	if  (outlen != 0)
		free((char *) coded);

	if  (!snmp_wait())
		return  snmp_ok = RES_OFFLINE;

	inlen = snmp_recv((char *) inbuf, sizeof(inbuf));
	if  (debug > 3)  {
		fprintf(stderr, "Incoming SNMP request...\n");
		prinbuf(inbuf, inlen);
	}

	snmp_ok = snmp_parse_result(inbuf, inlen, &last_result);
	if  (debug > 1 && snmp_ok == RES_OK)  {
		if  (last_result.res_id_string)
			fprintf(stderr, "Id = %s:\t", last_result.res_id_string);
		switch  (last_result.res_type)  {
		case  RES_TYPE_STRING:
			fprintf(stderr, "Result string \"%s\"\n", last_result.res_un.res_string);
			break;
		case  RES_TYPE_SIGNED:
			fprintf(stderr, "Result integer %ld\n", last_result.res_un.res_signed);
			break;
		case  RES_TYPE_UNSIGNED:
			fprintf(stderr, "Result unsigned %lu\n", last_result.res_un.res_unsigned);
			break;
		}
	}
	return  snmp_ok;
}

char  *eval_string(struct value *val)
{
	int	ch, ch2;
	long	nn;
	char	*strv, *rv, *cp, *rp, nbuf[20];

	switch  (val->type)  {
	case  STRING_VALUE:
		return  expand(val->val_un.stringval);

	case  FSTRING_VALUE:
		return  stracpy(val->val_un.stringval);

	case  NAME_VALUE:
		return  stracpy(val->val_un.namev->expansion);

	case  NUM_VALUE:
		sprintf(nbuf, "%ld", val->val_un.longval);
		return  stracpy(nbuf);

	case  CMD_STRING_VALUE:
		return  execute_str(val->val_un.stringval);

	case  CMD_NUM_VALUE:
		strv = execute_str(val->val_un.stringval);
		for  (cp = strv;  isspace(*cp);  cp++)
			;
		if  (!isdigit(*cp) && *cp != '-')  {
			if  (debug > 0)
				fprintf(stderr, "String %s returned non-numeric value %s\n", val->val_un.stringval, strv);
			nn = 0;
		}
		else
			nn = atol(cp);
		if  (debug > 1)
			fprintf(stderr, "String %s returned numeric value %ld\n", val->val_un.stringval, nn);
		free(strv);
		sprintf(nbuf, "%ld", nn);
		return  stracpy(nbuf);

	case  BRACE_VALUE:	/* GUESS WHO this is for */
		strv = execute_str(val->val_un.stringval);
		if  (!(rv = malloc((unsigned) (strlen(strv) + 1))))
			nomem();
		rp = rv;
		for  (cp = strv;  isspace(*cp);  cp++)
			;
		while  (isxdigit(*cp))  {
			ch = ghexdig(*cp++);
			ch2 = ghexdig(*cp++);
			if  (ch2 < 0)
				break;
			if  (*cp == ':')
				cp++;
			*rp++ = (ch << 4) | ch2;
		}
		*rp = '\0';
		free(strv);
		return  rv;

	case  SNMPVAR_VALUE:
		if  (eval_snmp(val->val_un.stringval) != RES_OK)  {
			if  (debug > 0)
				fprintf(stderr, "SNMP value fetch %s failed\n", val->val_un.stringval);
			return  stracpy("");
		}
		if  (last_result.res_type == RES_TYPE_STRING)
			return  stracpy(last_result.res_un.res_string);
		if  (last_result.res_type == RES_TYPE_SIGNED)  {
			if  (debug > 1)
				fprintf(stderr, "Converting SNMP value of %s from signed to string\n", val->val_un.stringval);
			sprintf(nbuf, "%ld", last_result.res_un.res_signed);
			return  stracpy(nbuf);
		}
		if  (debug > 1)
			fprintf(stderr, "Converting SNMP value of %s from unsigned to string\n", val->val_un.stringval);
		sprintf(nbuf, "%lu", last_result.res_un.res_unsigned);
		return  stracpy(nbuf);

	case  LASTVAL_VALUE:
		if  (snmp_ok != RES_OK)  {
			if  (debug > 0)
				fprintf(stderr, "Invalid last SNMP value\n");
			return  stracpy("");
		}
		if  (last_result.res_type == RES_TYPE_STRING)
			return  stracpy(last_result.res_un.res_string);
		if  (last_result.res_type == RES_TYPE_SIGNED)  {
			if  (debug > 1)
				fprintf(stderr, "Converting last SNMP value from signed to string\n");
			sprintf(nbuf, "%ld", last_result.res_un.res_signed);
			return  stracpy(nbuf);
		}
		if  (debug > 1)
			fprintf(stderr, "Converting last SNMP value from unsigned to string\n");
		sprintf(nbuf, "%lu", last_result.res_un.res_unsigned);
		return  stracpy(nbuf);

	default:
		if  (debug > 0)
			fprintf(stderr, "Undefined value type %d\n", val->type);
		return  stracpy("0");
	}
}

/* Get number.  This is a bit of a long way round but we only use it in one place */

long	eval_num(struct value *val)
{
	char	*strval, *cp;
	long	nn;

	switch  (val->type)  {
	case  STRING_VALUE:
	case  FSTRING_VALUE:
	case  NAME_VALUE:
	case  CMD_STRING_VALUE:
	case  CMD_NUM_VALUE:
	case  BRACE_VALUE:
		strval = eval_string(val);
		for  (cp = strval;  isspace(*cp);  cp++)
			;
		if  (!isdigit(*cp) && *cp != '-')  {
			if  (debug > 0)
				fprintf(stderr, "String %s returned non-numeric value %s\n", val->val_un.stringval, strval);
			free(strval);
			return  0L;
		}
		nn = atol(cp);
		free(strval);
		return  nn;

	case  NUM_VALUE:
		return  val->val_un.longval;

	case  SNMPVAR_VALUE:
		if  (eval_snmp(val->val_un.stringval) != RES_OK)  {
			if  (debug > 0)
				fprintf(stderr, "SNMP value fetch %s failed\n", val->val_un.stringval);
			return  0L;
		}
		if  (last_result.res_type == RES_TYPE_STRING)  {
			if  (debug > 0)
				fprintf(stderr, "SNMP value of %s is string\n", val->val_un.stringval);
			return  0L;
		}
		if  (last_result.res_type == RES_TYPE_SIGNED)
			return  last_result.res_un.res_signed;

		return  last_result.res_un.res_unsigned;

	case  LASTVAL_VALUE:
		if  (snmp_ok != RES_OK)  {
			if  (debug > 0)
				fprintf(stderr, "Invalid last SNMP value\n");
			return  0L;
		}
		if  (last_result.res_type == RES_TYPE_STRING)  {
			if  (debug > 0)
				fprintf(stderr, "Last SNMP value is string\n");
			return  0L;
		}
		if  (last_result.res_type == RES_TYPE_SIGNED)
			return  last_result.res_un.res_signed;
		return  last_result.res_un.res_unsigned;
	}

	return  0L;		/* Belt & braces */
}

/* This lookup table is set up to return 1 (true) or 0 (false) when
   indexed by a comparison (in the order given in xtlhpdefs.h for
   COMP_EQ etc) and a comparision result 0 for < 1 for == and 2
   for > */

static	const	unsigned  char	clook[6][3] =  {
	{ 0, 1, 0},		/* EQ */
	{ 1, 0, 1},		/* NE */
	{ 1, 0, 0},		/* LT */
	{ 1, 1, 0},		/* LE */
	{ 0, 0, 1},		/* GT */
	{ 0, 1, 1}};		/* GE */

int	exec_cond(struct compare *cmp)
{
	char	*str1, *str2;
	int	ret, indx;
	long	n1, n2;

	switch  (indx = cmp->type)  {
	default:
		indx = 0;	/* Better return something I suppose */
		ret = 1;
		break;

	case  COMP_EQ:
	case  COMP_NE:
	case  COMP_LT:
	case  COMP_LE:
	case  COMP_GT:
	case  COMP_GE:
		n1 = eval_num(cmp->left);
		n2 = eval_num(cmp->right);
		ret = n1 < n2? 0: n1 == n2? 1: 2;
		break;

	case  COMP_EQ+STR_COMP:
	case  COMP_NE+STR_COMP:
	case  COMP_LT+STR_COMP:
	case  COMP_LE+STR_COMP:
	case  COMP_GT+STR_COMP:
	case  COMP_GE+STR_COMP:
		indx -= STR_COMP;
		str1 = eval_string(cmp->left);
		str2 = eval_string(cmp->right);
		ret = strcmp(str1, str2);
		free(str1);
		free(str2);
		ret = ret < 0? 0: ret == 0? 1: 2;
		break;
	}
	return  (int) clook[indx][ret];
}

int	exec_bitop(struct boolexpr *expr)
{
	long	n1;
	int	b1, b2;

	n1 = eval_num(expr->rightexpr->left_un.val);
	if  (n1 < 0)
		return  0;
	if  (expr->left_un.snmpstring) /* Null if using LASTVAL */
		eval_snmp(expr->left_un.snmpstring); /* Sets snmp_ok */
	if  (snmp_ok != RES_OK)
		return  0;
	if  (last_result.res_type != RES_TYPE_STRING)
		return  0;
	b1 = 1 << (7 - n1 % 8);
	b2 = n1 / 8;
	return  last_result.res_un.res_string[b2] & b1;
}

int	exec_allclear(struct boolexpr *expr)
{
	char	*str;
	unsigned  l;

	if  (expr->left_un.snmpstring) /* Null if using LASTVAL */
		eval_snmp(expr->left_un.snmpstring); /* Sets snmp_ok */
	if  (snmp_ok != RES_OK)
		return  0;
	if  (last_result.res_type != RES_TYPE_STRING)
		return  0;
	str = last_result.res_un.res_string;
	for  (l = 0;  l < last_result.res_length;  l++)
		if  (str[l] != 0)
			return  0;
	return  1;
}

int	exec_expr(struct boolexpr *expr)
{
	switch  (expr->type)  {
	case  VARDEFINED:
		return  eval_snmp(expr->left_un.snmpstring) == RES_OK;

	case  VARUNDEFINED:
		return  eval_snmp(expr->left_un.snmpstring) != RES_OK;

	case  ISSTRINGVAL:
		switch  (expr->left_un.val->type)  {
		default:
		case  NUM_VALUE:
		case  CMD_NUM_VALUE:
			return  0;

		case  STRING_VALUE:
		case  FSTRING_VALUE:
		case  CMD_STRING_VALUE:
		case  NAME_VALUE:
		case  BRACE_VALUE:
			return  1;

		case  SNMPVAR_VALUE:
			return  eval_snmp(expr->left_un.val->val_un.stringval) == RES_OK  &&  last_result.res_type == RES_TYPE_STRING;

		case  LASTVAL_VALUE:
			return  snmp_ok == RES_OK  &&  last_result.res_type == RES_TYPE_STRING;
		}

	case  ISNUMVAL:
		switch  (expr->left_un.val->type)  {
		default:
		case  STRING_VALUE:
		case  FSTRING_VALUE:
		case  CMD_STRING_VALUE:
		case  NAME_VALUE:
		case  BRACE_VALUE:
			return  0;

		case  NUM_VALUE:
		case  CMD_NUM_VALUE:
			return  1;

		case  SNMPVAR_VALUE:
			return  eval_snmp(expr->left_un.val->val_un.stringval) == RES_OK  &&  last_result.res_type != RES_TYPE_STRING;

		case  LASTVAL_VALUE:
			return  snmp_ok == RES_OK  &&  last_result.res_type != RES_TYPE_STRING;
		}

	case  COMPARE:
		return  exec_cond(expr->left_un.comp);

	case  BITOPER:
		return  exec_bitop(expr);

	case  ALL_CLEAR:
		return  exec_allclear(expr);

	case  ANDEXPR:
		return  exec_expr(expr->left_un.expr)  &&  exec_expr(expr->rightexpr);

	case  OREXPR:
		return  exec_expr(expr->left_un.expr)  ||  exec_expr(expr->rightexpr);

	case  NOTEXPR:
		return  !exec_expr(expr->rightexpr);
	}

	return  0;
}

/* This does the biz.  */

int	exec_script(struct command *cmdlist)
{
	char	*expval;
	int	reti;
	long	ret;

	while  (cmdlist)  {
		switch  (cmdlist->type)  {
		case  CMD_NOP:
			break;

		case  CMD_ASS:
		case  CMD_ONEASS:
			expval = eval_string(cmdlist->cmd_un.ass.ass_value);
			if  (cmdlist->cmd_un.ass.ass_name->expansion)
				free(cmdlist->cmd_un.ass.ass_name->expansion);
			cmdlist->cmd_un.ass.ass_name->expansion = expval;
			if  (debug > 1)
				fprintf(stderr, "Assigned %s = %s\n", cmdlist->cmd_un.ass.ass_name->name, expval);
			if  (cmdlist->type == CMD_ONEASS)
				cmdlist->type = CMD_NOP;
			break;

		case  CMD_IF:
			if  (exec_expr(cmdlist->cmd_un.ifthen.comp_expr))
				reti = exec_script(cmdlist->cmd_un.ifthen.thenpart);
			else
				reti = exec_script(cmdlist->cmd_un.ifthen.elsepart);
			if  (reti >= 0)
				return  reti;
			break;

		case  CMD_EXIT:
			ret = eval_num(cmdlist->cmd_un.exitcode);
			if  (debug > 1)
				fprintf(stderr, "Exit %ld\n", ret);
			if  ((ret < 0  ||  ret > 255)  &&  stderr)  {
				fprintf(stderr, "Exit %ld????\n", ret);
				return  (int) (ret & 255L);
			}
			return  (int)ret;

		case  CMD_MSG:
			expval = eval_string(cmdlist->cmd_un.msgval);
			if  (debug > 1)
				fprintf(stderr, "Msg %s\n", expval);
			fprintf(stderr, "%s\n", expval);
			free(expval);
			break;

		case  CMD_FLUSH:
			do_flush();
			break;
		}
		cmdlist = cmdlist->next;
	}
	return  -1;
}
