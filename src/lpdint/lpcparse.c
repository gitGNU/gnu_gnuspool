/* lpcparse.c -- parse control file for xtlpc

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
#include <sys/types.h>
#include <ctype.h>
#include "incl_unix.h"
#include "lpctypes.h"

#define	HASHMOD	59

static	struct	varname	*hashtab[HASHMOD];

struct	ctrltype	*card_list, *proto_list;

static	int	line_count = 1;

void  nomem()
{
	fprintf(stderr, "Run out of memory\n");
	exit(255);
}

static void  err(char *msg)
{
	fprintf(stderr, "Control file error line %d: %s\n", line_count, msg);
}

static unsigned  hashcalc(const char *name)
{
	unsigned  result = 0;
	while  (*name)
		result = (result << 1) ^ *name++;
	return  result % HASHMOD;
}

struct varname	*lookuphash(const char *name)
{
	struct	varname	*hp;
	unsigned  hashval = hashcalc(name);

	for  (hp = hashtab[hashval];  hp;  hp = hp->vn_next)
		if  (strcmp(hp->vn_name, name) == 0)
			return  hp;
	if  (!(hp = (struct varname *) malloc(sizeof(struct varname))))
		nomem();
	hp->vn_next = hashtab[hashval];
	hashtab[hashval] = hp;
	hp->vn_name = stracpy(name);
	hp->vn_value = (char *) 0;
	return  hp;
}

static struct ctrltype *alloc_ctrl(struct ctrltype **where)
{
	struct	ctrltype  *np;
	struct	ctrltype  *result = (struct ctrltype *) malloc(sizeof(struct ctrltype));
	if  (!result)
		nomem();
	result->ctrl_next = (struct ctrltype *) 0;
	result->ctrl_cond = (struct condition *) 0;
	result->ctrl_string = (char *) 0;
	result->ctrl_action = CT_ASSIGN;
	for  (;  (np = *where);  where = &np->ctrl_next)
		;
	*where = result;
	return  result;
}

char *expandvars(char *str)
{
	unsigned  length = 1;
	char	*result, *rp, *sp = str;
	char	*varname = malloc((unsigned) (1+strlen(str)));
	char	nbuf[20];

	if  (!varname)
		nomem();

	/* Calculate length.  */

	while  (*sp)  {
		if  (*sp == '$')  {
			sp++;
			if  (*sp == '\0')
				break;
			if  (*sp == '$')  {
				sp++;
				sprintf(nbuf, "%ld", (long) getpid());
				length += strlen(nbuf);
			}
			else  {
				char	*vp = varname;
				struct	varname	*varp;
				do  *vp++ = *sp++;
				while  (isalnum(*sp));
				*vp = '\0';
				varp = lookuphash(varname);
				if  (varp->vn_value)
					length += strlen(varp->vn_value);
			}
		}
		else  {
			length++;
			sp++;
		}
	}

	if  (!(result = malloc(length)))
		nomem();

	rp = result;
	sp = str;

	/* Copy over text.  */

	while  (*sp)  {
		if  (*sp == '$')  {
			sp++;
			if  (*sp == '\0')
				break;
			if  (*sp == '$')  {
				sp++;
				sprintf(nbuf, "%ld", (long) getpid());
				strcpy(rp, nbuf);
				rp += strlen(nbuf);
			}
			else  {
				char	*vp = varname;
				struct	varname	*varp;
				do  *vp++ = *sp++;
				while  (isalnum(*sp));
				*vp = '\0';
				varp = lookuphash(varname);
				if  (varp->vn_value)  {
					strcpy(rp, varp->vn_value);
					rp += strlen(varp->vn_value);
				}
			}
		}
		else
			*rp++ = *sp++;
	}
	*rp = '\0';
	free(varname);
	return  result;
}

static struct condition *read_cond(FILE *inf)
{
	int	ch, cnt;
	char	linbuf[160];
	static	struct	condition	result;

	do  ch = getc(inf);
	while  (ch == ' ' || ch == '\t');

	if  (!isalpha(ch))  {
		err("Expecting variable name in cond");
		return  (struct condition *) 0;
	}
	cnt = 0;
	do  {
		if  (cnt < sizeof(linbuf) - 1)
			linbuf[cnt++] = (char) ch;
		ch = getc(inf);
	}  while  (isalnum(ch));
	linbuf[cnt] = '\0';
	result.cond_var = lookuphash(linbuf);

	while  (ch == ' ' || ch == '\t')
		ch = getc(inf);

	switch  (ch)  {
	default:
	badcond:
		err("Bad condition");
		return  (struct condition *) 0;

	case  ')':		/* Patch later */
	case  '}':
		result.cond_type = COND_DEF;
		result.cond_string = (char *) 0;
		return  &result;

	case  '=':
		do  ch = getc(inf);
		while  (ch == '=');
		result.cond_type = COND_EQ;
		goto  getstr;

	case  '~':
		do  ch = getc(inf);
		while  (ch == '=');
		result.cond_type = COND_MATCH;
		goto  getstr;

	case  '!':
		ch = getc(inf);
		if  (ch == '=')  {
			result.cond_type = COND_NEQ;
			goto  getstr;
		}
		if  (ch == '~')  {
			result.cond_type = COND_NONMATCH;
			goto  getstr;
		}
		err("Unknown operator");
		return  (struct condition *) 0;

	getstr:
		while  (ch == ' ' || ch == '\t')
			ch = getc(inf);
		cnt = 0;
		do  {
			if  (cnt < sizeof(linbuf) - 1)
				linbuf[cnt++] = (char) ch;
			ch = getc(inf);
		}  while  (ch != ')' && ch != '}' && ch != ' ' && ch != '\t' && ch != '\n' && ch != EOF);
		linbuf[cnt] = '\0';
		while  (ch == ' ' || ch == '\t')
			ch = getc(inf);
		if  (ch != ')'  &&  ch != '}')
			goto  badcond;

		/* There is a storage leak here in the assignments
		   section, but we won't worry about it until
		   later.  */

		result.cond_string = stracpy(linbuf);
		return  &result;
	}
}

int  evalcond(struct condition *cp)
{
	char	*varvalue;

	if  (!cp->cond_var)
		return  0;
	varvalue = cp->cond_var->vn_value;

	switch  (cp->cond_type)  {
	default:
		return  0;

	case  COND_DEF:
		return  varvalue  &&  varvalue[0];

	case  COND_EQ:
		return  varvalue  &&  strcmp(varvalue, cp->cond_string) == 0;

	case  COND_NEQ:
		return  !varvalue  ||  strcmp(varvalue, cp->cond_string) != 0;

	case  COND_MATCH:
		return  varvalue  &&  qmatch(cp->cond_string, varvalue);

	case  COND_NONMATCH:
		return  !(varvalue  &&  qmatch(cp->cond_string, varvalue));
	}
}

static struct condition *copycond(struct condition *con)
{
	struct	condition	*result;

	if  (!con)
		return  con;
	result = (struct condition *) malloc(sizeof(struct condition));
	if  (!result)
		nomem();
	*result = *con;
	return  result;
}

static char *getcard(FILE *inf, char *linbuf)
{
	int	ch, cnt;

	do  ch = getc(inf);
	while  (ch == ' ' || ch == '\t');

	cnt = 0;
	while  (ch != '\n'  &&  ch != EOF)  {
		if  (cnt < 159)
			linbuf[cnt++] = (char) ch;
		ch = getc(inf);
	}
	linbuf[cnt] = '\0';
	line_count++;
	return  linbuf;
}

int  parsecf(FILE *infile)
{
	int	ch, prepend, append, quote, cnt;
	char	*newstr, *card;
	struct	condition	*ccond;
	struct	ctrltype	*ctrlf;
	struct	varname		*varp;
	char	linbuf[160];

	for  (;;)  {

		/* Beginning of line.  */

		ccond = (struct condition *) 0;

		do  ch = getc(infile);
		while  (ch == ' '  ||  ch == '\t');

		if  (ch == EOF)
			break;
		if  (ch == '\n')  {
			line_count++;
			continue;
		}

		/* Comments...  */

		if  (ch == '#')  {
			do  ch = getc(infile);
			while  (ch != '\n'  &&  ch != EOF);
			line_count++;
			continue;
		}

		switch  (ch)  {
		default:
			err("Unknown command");
			return  1;

		case  '{':
			if  (!(ccond = read_cond(infile)))
				return  1;

			do  ch = getc(infile);
			while  (ch == ' '  ||  ch == '\t');

			if  (ch == ':')
				goto  grabcfile;

			if  (!isalpha(ch))  {
				err("Unknown conditional command");
				return  1;
			}

			/* Ignore rest of line if condition doesn't hold */

			if  (!evalcond(ccond))  {
				do  ch = getc(infile);
				while  (ch != '\n'  &&  ch != EOF);
				line_count++;
				continue;
			}

		case 'a':case 'b':case 'c':case 'd':case 'e':case 'f':
		case 'g':case 'h':case 'i':case 'j':case 'k':case 'l':
		case 'm':case 'n':case 'o':case 'p':case 'q':case 'r':
		case 's':case 't':case 'u':case 'v':case 'w':case 'x':
		case 'y':case 'z':
		case 'A':case 'B':case 'C':case 'D':case 'E':case 'F':
		case 'G':case 'H':case 'I':case 'J':case 'K':case 'L':
		case 'M':case 'N':case 'O':case 'P':case 'Q':case 'R':
		case 'S':case 'T':case 'U':case 'V':case 'W':case 'X':
		case 'Y':case 'Z':

			/* Must be variable name for assignment We do
			   this right away.  */

			cnt = 0;
			do  {
				if  (cnt < sizeof(linbuf) - 1)
					linbuf[cnt++] = (char) ch;
				ch = getc(infile);
			}  while  (isalnum(ch));

			linbuf[cnt] = '\0';
			varp = lookuphash(linbuf);

			while  (ch == ' '  ||  ch == '\t')
				ch = getc(infile);

			prepend = append = 0;

			/* Read assign operator += append =+ prepend */

			if  (ch == '+')  {
				append = 1;
				ch = getc(infile);
			}

			if  (ch != '=')  {
				err("Expecting \'=\'");
				return  1;
			}

			ch = getc(infile);
			if  (ch == '+')  {
				prepend = 1;
				ch = getc(infile);
			}

			/* Skip leading space */

			while  (ch == ' '  ||  ch == '\t')
				ch = getc(infile);

			quote = ch;

			switch  (ch)  {
			case  '\n':
				line_count++;
			case  EOF:
				if  (varp->vn_value)  {
					free(varp->vn_value);
					varp->vn_value = (char *) 0;
				}
				continue;

			default:
				cnt = 0;
				do  {
					if  (cnt < sizeof(linbuf) - 1)
						linbuf[cnt++] = (char) ch;
					ch = getc(infile);
				}  while  (ch != '\n' && ch != EOF);
				linbuf[cnt] = '\0';
				break;

			case  '\'':
			case  '\"':
				cnt = 0;
				for  (;;)  {
					ch = getc(infile);
					if  (ch == quote  ||  ch == '\n' || ch == EOF)
						break;
					if  (cnt < sizeof(linbuf) - 1)
						linbuf[cnt++] = (char) ch;
				}
				linbuf[cnt] = '\0';
				if  (ch != quote)  {
					err("Unterminated string");
					return  1;
				}
				do  ch = getc(infile);
				while  (ch == ' '  ||  ch == '\t');
				break;
			}

			newstr = expandvars(linbuf);

			if  (varp->vn_value  &&  (append || prepend))  {
				char	*res = malloc((unsigned) (strlen(newstr) + strlen(varp->vn_value) + 1));
				if  (!res)
					nomem();
				if  (append)
					sprintf(res, "%s%s", varp->vn_value, newstr);
				else
					sprintf(res, "%s%s", newstr, varp->vn_value);
				free(newstr);
				newstr = res;
			}

			if  (varp->vn_value)
				free(varp->vn_value);
			varp->vn_value = newstr;

			if  (ch == '\n')
				line_count++;
			continue;

		case  ':':
		grabcfile:
			if  (!(card = getcard(infile, linbuf)))
				return  1;

			ctrlf = alloc_ctrl(&card_list);
			ctrlf->ctrl_cond = copycond(ccond);
			ctrlf->ctrl_string = stracpy(card);
			ctrlf->ctrl_action = CT_ADDCFILE;
			continue;

		case  '(':
			if  (!(ccond = read_cond(infile)))
				return  1;
			if  (ccond->cond_type != COND_DEF)  {
				err("Invalid multiplier type");
				return  1;
			}
			ccond->cond_type = COND_MULT;
			do  ch = getc(infile);
			while  (ch == ' '  ||  ch == '\t');

			if  (ch == ':')
				goto  grabcfile;
			err("Unknown command after multiplier");
			return  1;

		case  '>':
		case  ']':
			if  (!(card = getcard(infile, linbuf)))
				return  1;
			ctrlf = alloc_ctrl(&proto_list);
			ctrlf->ctrl_string = stracpy(card);
			ctrlf->ctrl_action = ch == ']'? CT_SENDLINEONCE: CT_SENDLINE;
			continue;

		case  '<':
		case  '[':
			if  (!(card = getcard(infile, linbuf)))
				return  1;
			ctrlf = alloc_ctrl(&proto_list);
			ctrlf->ctrl_string = stracpy(card);
			ctrlf->ctrl_action = ch == '['? CT_RECVLINEONCE: CT_RECVLINE;
			continue;

		case  '*':
			if  (!(card = getcard(infile, linbuf)))
				return  1;
			ctrlf = alloc_ctrl(&proto_list);
			ctrlf->ctrl_string = stracpy(card);
			ctrlf->ctrl_action = CT_SENDFILE;
			continue;
		}
	}

	return  0;
}
