/* matchrouts.c -- glob style pattern match with , for alternatives

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

char *match_comma(char *patt)
{
	while  (*patt  &&  *patt != ',')  {
		if  (*patt == '[')  {
			do  patt++;
			while  (*patt  &&  *patt != ']');
		}
		if  (!*patt)
			return  (char *) 0;
		patt++;
	}
	if  (!*patt)
		return  (char *) 0;
	return  patt;
}

int  isinrange(const int ch, const char *patt, int *cnt)
{
	int	nott = 0;
	const	char	*patta = patt;

	if  (*++patta == '!')  {
		nott = 1;
		patta++;
	}

	/* Safety in case pattern truncated */

	if  (*patta == '\0')
		return  0;

	do  {
		int  lrange, hrange, lv, uv;

		/* Initialise limits of range */

		lrange = hrange = *patta++;
		if  (*patta == '-')  {
			hrange = *++patta;
			if  (hrange == 0) /* Safety in case trunacated */
				return  0;

			/* Be relaxed about backwards ranges */

			if  (hrange < lrange)  {
				int	tmp = hrange;
				hrange = lrange;
				lrange = tmp;
			}
			patta++; /* Past rhs of range */
		}

		/* If value matches, and we are excluding range, then
		   pattern doesn't and we quit.  Otherwise we
		   skip to the end.  */

		lv = uv = ch;
		if  (isupper(lv))
			lv = tolower(lv);
		if  (islower(uv))
			uv = toupper(uv);
		if  ((lv >= lrange  &&  lv <= hrange)  ||  (uv >= lrange  &&  uv <= hrange))  {
			if  (nott)
				return  0;
			while  (*patta  &&  *patta != ']')
				patta++;
			if  (*patta == '\0') /* Safety */
				return  0;
			patta++;
			*cnt = patta - patt;
			return  1;
		}
	}  while  (*patta  &&  *patta != ']');

	if  (*patta == '\0') /* Safety */
		return  0;
	while  (*patta++ != ']')
		;
	if  (!nott)
		return  0;
	*cnt = patta - patt;
	return  1;
}

int  repattok(const char *str)
{
	while  (*str)  {
		if  (*str == '[')  {
			do  {
				str++;
				if  (*str == ',' || *str == '[')
					return  0;
			}  while  (*str && *str != ']');
			if  (!*str)
				return  0;
		}
		str++;
	}
	return  1;
}
