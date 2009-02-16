/* matchrou.cpp -- match string routines

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

int		match_comma(const CString &patt)
{                     
	int	result = 0, lng = patt.GetLength();
	
	while  (result < lng  &&  patt[result] != ',')  {
		if  (patt[result] == '[')  {
			do  result++;
			while  (result < lng  &&  patt[result] != ']');
		}
		if  (result >= lng)
			return  -1;
		result++;
	}
	return  result >= lng? -1: result;
}

BOOL	isinrange(const int ch, const char *patt, int &cnt)
{
	int		nott = 0;
	const	char	*patta = patt;
	
	if  (*++patta == '!')  {
		nott = 1;
		patta++;
	}

	//	Safety in case pattern truncated

	if  (*patta == '\0')
		return  FALSE;

	do  {
		//	Initialise limits of range
		
		int	lrange = *patta++;
		int	hrange = lrange;
		
		if  (*patta == '-')  {
			hrange = *++patta;
			if  (hrange == 0) // Safety in case trunacated
				return  FALSE;

			//	Be relaxed about backwards ranges
			
			if  (hrange < lrange)  {
				int	tmp = hrange;
				hrange = lrange;
				lrange = tmp;
			}
			patta++; // Past rhs of range
		}

		//	If value matches, and we are excluding range, then
		//	pattern doesn't and we quit.
		//	Otherwise we skip to the end.

		int	lv = ch, uv = ch;
		
		if  (isupper(lv))
			lv = tolower(lv);
		if  (islower(uv))
			uv = toupper(uv);
		
		if  ((lv >= lrange  &&  lv <= hrange)  ||  (uv >= lrange  &&  uv <= hrange))  {
			if  (nott)
				return  FALSE;
			while  (*patta  &&  *patta != ']')
				patta++;
			if  (*patta == '\0') // Safety
				return  FALSE;
			patta++;
			cnt = patta - patt;
			return  TRUE;
		}
	}  while  (*patta  &&  *patta != ']');
	
	if  (*patta == '\0') // Safety
		return  FALSE;
	while  (*patta++ != ']')
		;
	if  (!nott)
		return  FALSE;
	cnt = patta - patt;
	return  TRUE;
}

BOOL	repattok(const  CString  &str)
{
	int	 cnt = 0, lng = str.GetLength();
	
	while  (cnt < lng)  {
		if  (str[cnt] == '[')  {
			do  {
				cnt++;
				if  (str[cnt] == ',' || str[cnt] == '[')
					return  FALSE;
			}  while  (str[cnt] && str[cnt] != ']');
			if  (cnt >= lng)
				return  FALSE;
		}
		cnt++;
	}
	return  TRUE;
}
