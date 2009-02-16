/* issubset.cpp -- Try to figure out if one regex is a subset of another

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

static	BOOL	ssmatch(const char *patta, const char *pattb)
{
	for  (;;)  {

		switch  (*patta)  {
		case  '\0':
			return  *pattb == '\0';

		default:
			if  (*patta != *pattb  &&  toupper(*patta) != toupper(*pattb))
				return  FALSE;
			patta++;
			pattb++;
			continue;

		case  '?':
			switch  (*pattb)  {
			case  '\0':
				return  FALSE;
			case  '*':
				return  FALSE;
			case  '[':		// More restrictive
				do  pattb++;
				while  (*pattb && *pattb != ']');
				if  (!*pattb)
					return  FALSE;
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
					return  TRUE;
				switch  (*pattb)  {
				default:
					pattb++;
					break;
				case  '[':
					do  pattb++;
					while  (*pattb && *pattb != ']');
					if  (!*pattb)
						return  FALSE;
					break;
				}
			}
			return  *patta == '\0';

		case  '[':
			if  (*pattb == '*' || *pattb == '?')
				return  FALSE;
			if  (*pattb != '[')  {
				int	cnt;
				if  (isinrange(*pattb, patta, cnt))  {
					pattb++;
					patta += cnt;
					continue;
				}
				return  FALSE;
			}
			else  {
				int	cnta, cntb;
				for  (int  a = ' ';  a <= '~';  a++)  {
					if  (isinrange(a, patta, cnta))
						continue;
					if  (isinrange(a, pattb, cntb))
						return  FALSE;
				}
				patta += cnta;
				pattb += cntb;
				continue;
			}
		}
	}
}

static	BOOL	smatcha(const CString &patterna, const char *patternb)
{
	int		res, cp;             
	CString	pp = patterna;
	
	do  {	// Loop over patterna
		cp = match_comma(pp);
		if  (cp >= 0)  {
			res = ssmatch((const char *) pp.Left(cp), patternb);
			pp = pp.Mid(cp+1);
		}
		else 
			res = ssmatch((const char *) pp, patternb);
		if  (res)
			return  TRUE;
	}  while  (cp >= 0);

	return  FALSE;
}	

BOOL	issubset(CString &patterna, CString &patternb)
{
	int		res, cp;
	CString	pp = patternb;

	do  {	// Loop over patternb
		cp = match_comma(pp);
		if  (cp >= 0)  {
			res = smatcha(patterna, (const char *) pp.Left(cp));
			pp = pp.Mid(cp+1);
		}
		else
			res = smatcha(patterna, (const char *) pp);
		if  (!res)
			return  FALSE;
	}  while  (cp);

	return  TRUE;
}
