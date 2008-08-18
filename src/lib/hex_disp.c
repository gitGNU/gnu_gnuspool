/* hex_disp.c -- turn class codes into string

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
#include "defaults.h"

/* Return class code as string.
   Second argument is non-zero to indicate that all bits should be
   represented with 0 as '.' */

char  *hex_disp(const classcode_t value, const int plustrail)
{
	int	i;
	static	char	result[33];
	char	*rp = result;

	if  (plustrail)  {
		for  (i = 0;  i < 16;  i++)
			*rp++ = (value & (1 << i))? 'A' + i: '.';
		for  (;  i < 32;  i++)
			*rp++ = (value & (1 << i))? 'a' - 16 + i: '.';
	}
	else  {
		int	bitcnt, pi;
		for  (i = 0;  i < 16;  i++)  {
			if  (value & (1 << i))  {
				bitcnt = 1;
				i++;
				while  (i < 16  &&  value & (1 << i))  {
					i++;
					bitcnt++;
				}
				if  (bitcnt > 3)  {
					*rp++ = 'A' + i - bitcnt;
					*rp++ = '-';
					*rp++ = 'A' + i - 1;
				}
				else  for  (pi = i - bitcnt;  pi < i;  pi++)
					*rp++ = 'A' + pi;
			}
		}
		for  (i = 16;  i < 32;  i++)  {
			if  (value & (1 << i))  {
				bitcnt = 1;
				i++;
				while  (i < 32  &&  value & (1 << i))  {
					i++;
					bitcnt++;
				}
				if  (bitcnt > 3)  {
					*rp++ = 'a' + i - bitcnt - 16;
					*rp++ = '-';
					*rp++ = 'a' + i - 17;
				}
				else  for  (pi = i - bitcnt;  pi < i;  pi++)
					*rp++ = 'a' + pi - 16;
			}
		}
	}
	*rp = '\0';
	return  result;
}
