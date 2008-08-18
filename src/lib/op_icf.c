/* op_icf.c -- find and open server process help file

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
#include <sys/types.h>
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include "defaults.h"
#include "files.h"
#include "incl_unix.h"
#include "cfile.h"

extern	char	*Helpfile_path;

FILE *	open_icfile(void)
{
	char	*filename;
	FILE	*res;

	filename = envprocess(INT_CONFIG);
	if  ((res = fopen(filename, "r")) == (FILE *) 0)  {
		fprintf(stderr,
			"Help cannot open internal config file `%s'\n",
			filename);
		return  (FILE *) 0;
	}
	Helpfile_path = filename;
	fcntl(fileno(res), F_SETFD, 1);
	return  res;
}
