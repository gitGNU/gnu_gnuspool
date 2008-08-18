/* stracpy.c -- return an allocated copy of a string

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
#include "incl_unix.h"

/* Yes I have heard of strdup but the versions of Unix on which this
   was first implemented back in 1984 hadn't. Yes I'll change it later.
   Anyway I want to call nomem() if we're out of memory */

char  *stracpy(const char *s)
{
	unsigned  l = strlen(s) + 1;
	char	*r;
	if  ((r = (char *) malloc(l)) == (char *) 0)
		nomem();
	return	strcpy(r, s);
}
