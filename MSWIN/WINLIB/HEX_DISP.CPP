/* hex_disp.cpp -- display class codes

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
                                  
char	*hex_disp(const classcode_t value)
{
#ifdef	PREREL22
	static	char	result[17];
	for  (int i = 0;  i < 16;  i++)
		result[i] = (value & (1 << i))? 'A' + i: '.';

	return  result;
#else
	static	char	result[33];
	for  (int i = 0;  i < 16;  i++)
		result[i] = (value & (1L << i))? 'A' + i: '.';
	for  (i = 0;  i < 16;  i++)
		result[i] = (value & (1L << (i+16)))? 'a' + i: '.';
	return  result;
#endif
}
