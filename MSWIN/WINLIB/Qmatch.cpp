/* Qmatch.cpp -- match glob-like

   Copyright 2009 Free Software Foundation, Inc.

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

#include "stdafx.h"
#include "defaults.h"
#include <ctype.h>                                  

extern	BOOL	isinrange(const int, const char *, int &);
extern	int		match_comma(const CString &);

static	BOOL	ematch(const char *pattern, const char FAR *value)
{
	int	cnt;

	for  (;;)  {

		switch  (*pattern)  {
		case  '\0':
			if  (*value == '\0')
				return  TRUE;
			return  FALSE;

		default:
			if  (*pattern != *value  &&  toupper(*pattern) != toupper(*value))
				return  FALSE;
			pattern++;
			value++;
			continue;

		case  '?':
			if  (*value == '\0')
				return  FALSE;
			pattern++;
			value++;
			continue;

		case  '*':
			pattern++;
			for  (cnt = strlen(value); cnt >= 0;  cnt--)
				if  (ematch(pattern, value+cnt))
					return  TRUE;
			return  FALSE;

		case  '[':
			if  (*value == '\0')
				return  FALSE;
			if  (!isinrange(*value, pattern, cnt))
				return  FALSE;
			value++;
			pattern += cnt;
			continue;
		}
	}
}

BOOL	qmatch(CString &pattern, const char FAR *value)
{
	BOOL	res;
	int		cp;
	CString	pp = pattern;

	do  {
		cp = match_comma(pp);
		if  (cp >= 0)  {
			res = ematch((const char *) pp.Left(cp), value);
			pp = pp.Mid(cp+1);
		}
		else
			res = ematch((const char *) pp, value);
		if  (res)
			return  TRUE;
	}  while  (cp >= 0);

	//	Not found...
	return  FALSE;
}
