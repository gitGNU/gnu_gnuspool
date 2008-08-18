/* lpdparse.c -- parse xtlpd control file

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
#include "lpdtypes.h"

static	int	line_count = 1;

struct	ctrltype	*ctrl_list[127 - ' '];
struct	ctrltype	*begin_ctrl,
			*end_ctrl,
			*repeat_ctrl,
			*norepeat_ctrl;

#define	HASHMOD	59

static	struct	varname	*hashtab[HASHMOD];

void	nomem(void)
{
	fprintf(stderr, "Run out of memory\n");
	exit(255);
}

static void	err(char * msg)
{
	fprintf(stderr, "Control file error line %d: %s\n", line_count, msg);
}

static unsigned	hashcalc(const char * name)
{
	unsigned  result = 0;
	while  (*name)
		result = (result << 1) ^ *name++;
	return  result % HASHMOD;
}

struct varname	*	lookuphash(const char * name)
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

static struct ctrltype *	alloc_ctrl(int typ)
{
	struct	ctrltype	*result, **wf, *np;

	if  (!(result = (struct ctrltype *) malloc(sizeof(struct ctrltype))))
		nomem();
	result->ctrl_next = (struct ctrltype *) 0;
	result->ctrl_letter = (char) typ;

	if  (typ == '*')
		wf = &repeat_ctrl;
	else  if  (typ == '.')
		wf = &norepeat_ctrl;
	else  if  (typ == '!')
		wf = &begin_ctrl;
	else  if  (typ == '@')
		wf = &end_ctrl;
	else
		wf = &ctrl_list[typ - ' '];

	/* Put at end of the chain for letter */

	for  (;  (np = *wf);  wf = &np->ctrl_next)
		;
	*wf = result;
	return  result;
}

#define	INIT_BUF	1023
#define	INC_BUF		512

static char *	capture(char * str)
{
	FILE	*infile;
	char	*inbuf = malloc(INIT_BUF+1);
	unsigned	bufsize = INIT_BUF, bufcnt = 0;

	if  (!inbuf)
		nomem();

	if  ((infile = popen(str, "r")))  {
		int	ch;
		while  ((ch = getc(infile)) != EOF)  {
			if  (bufcnt >= bufsize)  {
				bufsize += INC_BUF;
				inbuf = realloc(inbuf, bufsize+1);
				if  (!inbuf)
					nomem();
			}
			inbuf[bufcnt++] = (char) ch;
		}
		while  (bufcnt != 0  &&  inbuf[bufcnt-1] == '\n')
			bufcnt--;
		inbuf[bufcnt] = '\0';
	}
	pclose(infile);
	return  inbuf;
}

char *	expandvars(char * str)
{
	unsigned  length = 1;
	char	*result, *rp, *sp, *varname;
	char	nbuf[20];

	if  (!(sp = str))  {
		if  (!(varname = malloc(1)))
			nomem();
		varname[0] = '\0';
		return  varname;
	}

	varname = malloc((unsigned) (1+strlen(str)));
	if  (!varname)
		nomem();

	/* Calculate length.  */

	while  (*sp)  {
		if  (*sp == '\\')  { /* Escape single character */
			length++;
			sp++;
			if  (*sp == '\0')
				break;
			length++;
			sp++;
		}
		else  if  (*sp == '\''  ||  *sp == '\"')  {
			int	quote = *sp++;
			length++;
			while  (*sp  &&  *sp != quote)  {
				length++;
				sp++;
			}
			if  (*sp)  {
				sp++;
				length++;
			}
		}
		else  if  (*sp == '$')  {
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
		if  (*sp == '\\')  { /* Escape single character */
			*rp++ = *sp++;
			if  (*sp == '\0')
				break;
			*rp++ = *sp++;
		}
		else  if  (*sp == '\''  ||  *sp == '\"')  {
			int	quote = *sp;
			*rp++ = *sp++;
			while  (*sp  &&  *sp != quote)
				*rp++ = *sp++;
			if  (*sp)
				*rp++ = *sp++;
		}
		else  if  (*sp == '$')  {
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

int	parsecf(FILE * infile)
{
	int	ch, prepend, append, quote, cnt;
	char	*newstr;
	struct	ctrltype	*ctrlf;
	struct	varname		*varp;
	char	linbuf[160];

	for  (;;)  {

		/* Beginning of line.  */

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

		if  (isalnum(ch))  {
			cnt = getc(infile);
			ungetc(cnt, infile);
			if  (cnt == '<')
				goto  ctrlchar;

			/* Must be variable name for assignment */

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
			case  '`':
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
			if  (quote == '`')  {
				char	*str = capture(newstr);
				free(newstr);
				newstr = str;
			}

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
		}

		/* * Control field */

		if  (ch != '.'  &&  ch != '*'  &&  ch != '!'  &&  ch != '@')  {
			err("Unknown command");
			return  1;
		}

	ctrlchar:		/* Join here for letter commands */

		ctrlf = alloc_ctrl(ch);
		ctrlf->ctrl_action = CT_NONE;
		ctrlf->ctrl_repeat = 0;
		ctrlf->ctrl_string = (char *) 0;
		ctrlf->ctrl_var = (struct varname *) 0;

		ch = getc(infile);
		if  (ctrlf->ctrl_letter != '!'  &&  ctrlf->ctrl_letter != '@')  {
			if  (ch != '<')  {
				err("Unknown command expecting \'<\'");
				return  1;
			}
			ch = getc(infile);
			switch  (ch)  {
			default:
				err("Unknown field type");
				return  1;
			case  'n':case  'N':
				ctrlf->ctrl_fieldtype = CT_NUMBER;
				break;
			case  's':case  'S':
				ctrlf->ctrl_fieldtype = CT_STRING;
				break;
			case  'f':case  'F':
				ctrlf->ctrl_fieldtype = CT_FILENAME;
				break;
			}
			ch = getc(infile);
			if  (ch != '>')  {
				err("Invalid command expecting \'>\'");
				return  1;
			}
			ch = getc(infile);
			if  (ch == '*')  {
				ctrlf->ctrl_repeat = 1;
				ch = getc(infile);
			}
		}
		if  (ch != ':')  {
			err("Invalid command expecting \':\'");
			return  1;
		}
		do  ch = getc(infile);
		while  (ch == ' '  ||  ch == '\t');

		switch  (ch)  {
		default:
			err("Unknown conditional command");
			return  1;
		case  '\n':
			line_count++;
		case  EOF:
			continue;
		case  '<':
			ch = getc(infile);
			switch  (ch)  {
			default:
				err("Unknown internal command");
				return  1;
			case  'i':case  'I':
				ctrlf->ctrl_action = CT_INDENT;
				break;
			case  'u':case  'U':
				ctrlf->ctrl_action = CT_UNLINK;
				break;
			}
			do  ch = getc(infile);
			while  (isalpha(ch));
			if  (ch != '>')  {
				err("Unknown command expecting \'>\'");
				return  1;
			}
			do  ch = getc(infile);
			while  (ch == ' ' || ch == '\t');
			if  (ctrlf->ctrl_action == CT_INDENT)  {
				if  (isdigit(ch))  {
					int	num = 0;
					char	numb[20];
					ctrlf->ctrl_fieldtype = CT_STRING;
					do  {
						num = num * 10 + ch - '0';
						ch = getc(infile);
					}  while  (isdigit(ch));
					sprintf(numb, "%d", num);
					ctrlf->ctrl_string = stracpy(numb);
				}
				else
					ctrlf->ctrl_fieldtype = CT_NUMBER;
			}
			else  {
				cnt = 0;
				do  {
					if  (cnt < sizeof(linbuf) - 1)
						linbuf[cnt++] = (char) ch;
					ch = getc(infile);
				}  while  (ch != '\n' && ch != EOF);
				linbuf[cnt] = '\0';
				ctrlf->ctrl_fieldtype = CT_STRING;
				ctrlf->ctrl_string = stracpy(linbuf);
			}
			while  (ch != '\n'  &&  ch != EOF)
				ch = getc(infile);
			line_count++;
			continue;

		case  '(':
			do  ch = getc(infile);
			while  (ch == ' ' || ch == '\t');
			if  (ch == '\n'  ||  ch == EOF)  {
				err("Empty ()| command");
				return  1;
			}
			cnt = 0;
			while  (isalnum(ch))  {
				if  (cnt < sizeof(linbuf) - 1)
					linbuf[cnt++] = (char) ch;
				ch = getc(infile);
			}
			linbuf[cnt] = '\0';
			if  (cnt <= 0)  {
				err("Missing name in ()s");
				return  1;
			}
			while  (ch == ' ' || ch == '\t')
				ch = getc(infile);
			if  (ch != ')')  {
				err("Missing )");
				return  1;
			}
			ctrlf->ctrl_var = lookuphash(linbuf);
			do  ch = getc(infile);
			while  (ch == ' ' || ch == '\t');
			if  (ch != '|')   {
				err("Missing |");
				return  1;
			}

		case  '|':
			do  ch = getc(infile);
			while  (ch == ' ' || ch == '\t');
			cnt = 0;
			if  (ch == '\n'  ||  ch == EOF)  {
				err("Empty | command");
				return  1;
			}
			do  {
				if  (cnt < sizeof(linbuf) - 1)
					linbuf[cnt++] = (char) ch;
				ch = getc(infile);
			}  while  (ch != '\n' && ch != EOF);
			linbuf[cnt] = '\0';
			ctrlf->ctrl_action = CT_PIPECOMMAND;
			ctrlf->ctrl_string = stracpy(linbuf);
			line_count++;
			continue;

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

			cnt = 0;
			do  {
				if  (cnt < sizeof(linbuf) - 1)
					linbuf[cnt++] = (char) ch;
				ch = getc(infile);
			}  while  (isalnum(ch));

			linbuf[cnt] = '\0';
			ctrlf->ctrl_var = lookuphash(linbuf);

			while  (ch == ' '  ||  ch == '\t')
				ch = getc(infile);

			ctrlf->ctrl_action = CT_ASSIGN;

			/* Read assign operator += append =+ prepend */

			if  (ch == '+')  {
				ctrlf->ctrl_action = CT_POSTASSIGN;
				ch = getc(infile);
			}

			if  (ch != '=')  {
				err("Expecting \'=\'");
				return  1;
			}

			ch = getc(infile);
			if  (ch == '+')  {
				ctrlf->ctrl_action = CT_PREASSIGN;
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

			ctrlf->ctrl_string = stracpy(linbuf);
			line_count++;
			continue;
		}
	}
	return  0;
}
