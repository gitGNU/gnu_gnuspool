/* mkspid.c -- make spool file name from job number

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
#include "defaults.h"
#include "files.h"
#include "network.h"
#include "spq.h"
#include "incl_unix.h"

static	char	result[NAMESIZE+4+1];

int	spid_subdirs = -1;

char  *mkspid(const char *nam, const jobno_t jnum)
{
	if  (spid_subdirs < 0)  {
		char	*ss = envprocess("${SPOOLSUBDS-0}");
		spid_subdirs = atoi(ss);
		if  (spid_subdirs < 0 || spid_subdirs > 999)
			spid_subdirs = 0;
		free(ss);
	}
	if  (spid_subdirs > 0)
		sprintf(result, "%.3lu/%s%.8lu", (unsigned long) (jnum % spid_subdirs), nam, (unsigned long) jnum);
	else
		sprintf(result, "%s%.8lu", nam, (unsigned long) jnum);
	return	result;
}
