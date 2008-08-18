/* doenv.c -- integrate options from environment vars with config handling

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
#include <sys/types.h>
#include <stdio.h>
#include "helpargs.h"
#include "incl_unix.h"

void  doenv(char *env, HelpargRef Adesc, optparam *const optlist, int minstate)
{
	char	**evec;

	if  (env == (char *) 0)
		return;
	evec = makevec(env);
	doopts(evec, Adesc, optlist, minstate);
	free((char *) evec[0]);
	free((char *) evec);
}
