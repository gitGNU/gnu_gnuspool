/* issubset.c -- decide if pattern is subset of another pattern

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
#include <setjmp.h>
#include "incl_unix.h"

static	jmp_buf	getaway;

extern int	isinrange(const int, const char *, int *);
extern char	*match_comma(char *);

static int	ssmatch(char *patta, char *pattb) /* TRUE if patta >= pattb */
{
	for  (;;)  {

		switch  (*patta)  {
		case  '\0':
			return  *pattb == '\0'? 1: 0;

		default:
			if  (*patta != *pattb  &&
			     (!(isalpha(*patta) && isalpha(*pattb)) ||  toupper(*patta) != toupper(*pattb)))
				return  0;
			patta++;
			pattb++;
			continue;

		case  '?':
			switch  (*pattb)  {
			case  '\0':
				longjmp(getaway, 1);
			case  '*':
				return  0;
			case  '[':		/* More restrictive */
				do  pattb++;
				while  (*pattb && *pattb != ']');
				if  (!*pattb)
					longjmp(getaway, 1);
				break;
			}
			patta++;
			pattb++;
			continue;

		case  '*':
			do  patta++;
			while  (*patta == '*');
			while  (*pattb)  {
				if  (ssmatch(patta, pattb))
					return  1;
				switch  (*pattb)  {
				default:
					pattb++;
					break;
				case  '[':
					do  pattb++;
					while  (*pattb && *pattb != ']');
					if  (!*pattb)
						longjmp(getaway, 1);
					break;
				}
			}
			return  *patta == '\0'? 1: 0;

		case  '[':
			if  (*pattb == '*' || *pattb == '?')
				return  0;
			if  (*pattb != '[')  {
				int	cnt;
				if  (isinrange(*pattb, patta, &cnt))  {
					pattb++;
					patta += cnt;
					continue;
				}
				return  0;
			}
			else  {
				int	a, cnta, cntb;
				for  (a = ' ';  a <= '~';  a++)  {
					if  (isinrange(a, patta, &cnta))
						continue;
					if  (isinrange(a, pattb, &cntb))
						return  0;
				}
				patta += cnta;
				pattb += cntb;
				continue;
			}
		}
	}
}

static int	smatcha(char *patterna, char *patternb)
{
	int	res;
	char	*cp;

	do  {	/* Loop over patterna */

		cp = match_comma(patterna);
		if  (cp)  {
			*cp = '\0';
			if  (setjmp(getaway))
				res = 0;
			else
				res = ssmatch(patterna, patternb);
			*cp = ',';
			patterna = cp + 1;
		}
		else  {
			if  (setjmp(getaway))
				return  0;
			res = ssmatch(patterna, patternb);
		}
		if  (res)
			return  1;
	}  while  (cp);

	return  0;
}

int	issubset(char *patterna, char *patternb)
{
	int	res;
	char	*cp;

	do  {	/* Loop over patternb */

		cp = match_comma(patternb);
		if  (cp)  {
			*cp = '\0';
			res = smatcha(patterna, patternb);
			*cp = ',';
			patternb = cp + 1;
		}
		else
			res = smatcha(patterna, patternb);
		if  (!res)
			return  0;
	}  while  (cp);

	return  1;
}
