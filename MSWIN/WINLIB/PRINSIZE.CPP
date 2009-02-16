/* prinsize.cpp -- print size with K M etc

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

const	struct  dtbl	{
	long	compar;
	int	shift;
	char	*fmt;
}  dtab[] = {
	{ 10000L, 0, "%ld" },
	{ 10L << 20, 10, "%ldK" },
	{ 1L << 30, 20, "%ldM" },
	{ 0L, 30, "%ldG" }
};

void	prin_size(char FAR * resbuf, const long l)
{
	for  (dtbl const *dt = dtab;  dt->compar &&  l >= dt->compar;  dt++)
		;
    
    long  cl = l;
	if  (dt->shift)
		cl = (cl + (1L << (dt->shift-1))) >> dt->shift;

	(void) wsprintf(resbuf, dt->fmt, cl);
}
