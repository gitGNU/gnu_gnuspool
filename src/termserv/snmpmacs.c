/* snmpmacs.c -- handle macros for names of SNMP ops

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

extern	int	debug;

#define	HASHMOD	509

static	struct	macro	*hashtab[HASHMOD];

void	nomem(void)
{
	fprintf(stderr, "Run out of memory\n");
	exit(255);
}

static unsigned	calchash(const char * name)
{
	unsigned	result = 0;
	while  (*name)
		result ^= (result << 1) | (result >> 31) | (unsigned) *name++;
	return  result % HASHMOD;
}

struct macro *	lookupname(const char * name)
{
	unsigned  hashval = calchash(name);
	struct	macro	*hp;

	for  (hp = hashtab[hashval]; hp;  hp = hp->next)
		if  (strcmp(name, hp->name) == 0)
			return  hp;
	return  (struct macro *) 0;
}

struct	macro *	lookupcreatename(const char *name)
{
	unsigned  hashval = calchash(name);
	struct	macro	*hp, **hpp;

	for  (hpp = &hashtab[hashval]; (hp = *hpp);  hpp = &hp->next)
		if  (strcmp(name, hp->name) == 0)
			return  hp;
	if  (!(hp = (struct macro *) malloc(sizeof(struct macro))))
		nomem();
	hp->next = (struct macro *) 0;
	hp->name = stracpy(name);
	hp->expansion = (char *) 0;
	*hpp = hp;
	return  hp;
}

int	ParseMacroFile(const char *name)
{
	FILE	*ifl;
	int	ch, cnt, startquote;
	struct	macro	*hp;
	char	nbuf[MAXNAMESIZE+1], ebuf[MAXEXPSIZE], enbuf[MAXNAMESIZE+1];;

	if  (!(ifl = fopen(name, "r")))
		return  0;

	for    (;;)  {

		/* Eat any preceding white space */

		do  ch = getc(ifl);
		while  (isspace(ch));

		if  (ch == EOF)
			break;

		startquote = 0;
		if  (ch == '\"'  ||  ch == '\'')  {
			startquote = ch;
			ch = getc(ifl);
		}
		if  (!isalpha(ch))  {
	eatline:
			while  (ch != '\n'  &&  ch != EOF)
				ch = getc(ifl);
			continue;
		}

		/* Start of name string */

		cnt = 0;
		do  {
			if  (cnt < MAXNAMESIZE)
				nbuf[cnt++] = ch;
			ch = getc(ifl);
		}  while  (isalnum(ch));

		nbuf[cnt] = '\0';

		if  (startquote)  {
			if  (ch != startquote)
				goto  eatline;
			ch = getc(ifl);
		}

		if  (!isspace(ch))
			goto  eatline;

		/* Look for expansion */

		do  ch = getc(ifl);
		while  (isspace(ch));

		startquote = 0;
		if  (ch == '\"'  ||  ch == '\'')  {
			startquote = ch;
			ch = getc(ifl);
		}

		/* Allow definition to start with something we've already defined */

		if  (!isalnum(ch))
			goto  eatline;

		cnt = 0;

		if  (isalpha(ch))  {
			do  {
				if  (cnt < MAXNAMESIZE)
					enbuf[cnt++] = ch;
				ch = getc(ifl);
			}  while  (isalnum(ch));

			enbuf[cnt] = '\0';
			if  (!(hp = lookupname(enbuf)))  {
				fprintf(stderr, "Warning: %s contains undefined macro %s\n", name, enbuf);
				goto  eatline;
			}

			strcpy(ebuf, hp->expansion);
			cnt = strlen(ebuf);
		}

		while  (isdigit(ch) ||  ch == '.')  {
			if  (cnt < MAXEXPSIZE)
				ebuf[cnt++] = ch;
			ch = getc(ifl);
		}

		if  (startquote)  {
			if  (ch != startquote)
				goto  eatline;
			ch = getc(ifl);
		}
		ebuf[cnt] = '\0';

		if  (cnt == 0  ||  ebuf[cnt] == '.')
			goto  eatline;

		hp = lookupcreatename(nbuf);
		if  (hp->expansion)
			free(hp->expansion);
		hp->expansion = stracpy(ebuf);
	}

	fclose(ifl);
	return  1;
}

/* Set up initial definitions.  */

void	init_define(const char *name, const char *val)
{
	struct	macro	*hp = lookupcreatename(name);
	if  (hp->expansion)
		free(hp->expansion);
	hp->expansion = stracpy(val);
}

/* This routine expands a string possibly containing macro names */

char  *expand(const char *value)
{
	const  char  *origvalue = value;
	char	*ep;
	int	ncnt;
	struct	macro	*hp;
	char	nbuf[MAXNAMESIZE+1], expbuf[BLDBUFF];

	ep = expbuf;

	/* Silently drop out if we overflow the buffer expbuf (or we
	   could do).  We assume that nothing will expand to
	   bigger than MAXEXPSIZE */

	while  (*value  &&  ep - expbuf  < BLDBUFF - MAXEXPSIZE)  {
		if  (isalpha(*value) || *value == '_')  {

			/* We possibly have a name Slurp it up.  */

			ncnt = 0;
			do  {
				if  (ncnt < MAXNAMESIZE)
					nbuf[ncnt++] = *value;
				value++;
			}  while  (isalnum(*value) || *value == '_');
			nbuf[ncnt] = '\0';

			/* Look up the name, if we know it, then copy
			   expansion into buffer in place of it,
			   otherwise false alarm, copy the name
			   instead.  */

			if  ((hp = lookupname(nbuf))  &&  hp->expansion)
				ep += strlen(strcpy(ep, hp->expansion));
			else
				ep += strlen(strcpy(ep, nbuf));
		}
		else		/* Some other char */
			*ep++ = *value++;
	}
	*ep = '\0';

	if  (debug > 2 || (debug > 1  && strcmp(origvalue, expbuf) != 0))
		fprintf(stderr, "%s expanded to %s\n", origvalue, expbuf);

	return  stracpy(expbuf);
}
