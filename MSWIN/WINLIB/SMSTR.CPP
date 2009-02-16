/* smstr.cpp -- string compare

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
#include <string.h>
#include <ctype.h>

BOOL  Smstr(const CString &str, const char *comp, const BOOL exact)
{
	int	 l = str.GetLength(), cl = strlen(comp);
	if  (cl < l)
		return  FALSE;

	if  (exact)  {
		if  (cl != l)
			return  FALSE;
		for  (int  cnt = 0;  cnt < l;  cnt++)
			if  (str[cnt] != '.' && toupper(str[cnt]) != toupper(comp[cnt]))
				return  FALSE;
		return  TRUE;
	}
	else  {
		int	 resid = cl - l;
		for  (int bcnt = 0;  bcnt <= resid;  bcnt++)  {
			for  (int  cnt = 0;  cnt < l ;  cnt++)  {
				if  (str[cnt] != '.' && toupper(str[cnt]) != toupper(comp[bcnt+cnt]))
					goto  notmatch;
			}
			return  TRUE;
		notmatch:
			;
		}
		return  FALSE;
	} 
}	
