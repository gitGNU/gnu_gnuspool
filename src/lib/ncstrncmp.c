/* ncstrncmp.c -- case insensitive string compare

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

/* Yes I have heard of strcasecmp etc but the versions of Unix on which this
   was first implemented back in 1984 hadn't. Yes I'll change it later */

int	ncstrcmp(const char *a, const char *b)
{
	int	ac, bc;

	for  (;;)  {
		ac = toupper(*a);
		bc = toupper(*b);
		if  (ac == 0  ||  bc == 0  ||  ac != bc)
			return  ac - bc;
		a++;
		b++;
	}
}

int  ncstrncmp(const char *a, const char *b, int n)
{
	int	ac, bc;

	while  (--n >= 0)  {
		ac = *a++;
		bc = *b++;
		if  (ac == 0  ||  bc == 0)
			return  ac - bc;
		if  (islower(ac))
			ac += 'A' - 'a';
		if  (islower(bc))
			bc += 'A' - 'a';
		if  (ac != bc)
			return  ac - bc;
	}
	return  0;
}
