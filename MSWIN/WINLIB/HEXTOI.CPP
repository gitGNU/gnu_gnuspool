/* hextoi.cpp -- translate class codes

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
#include <ctype.h>
#include "defaults.h"

classcode_t	hextoi(const char *arg)
{
	classcode_t  result  =  0;

	while  (*arg)  {
		if  (isalpha(*arg))  {
			int	ch;
			if  (toupper(*arg) > 'P')
				return  result;
			if  (isupper(*arg))
				ch = *arg++ - 'A';
			else
				ch = *arg++ - 'a' + 16;
			result |= 1L << ch;
		}
		else  if  (*arg == '.' || *arg == '-')
			arg++;
		else
			return  result;
	}
	return  result;
}
