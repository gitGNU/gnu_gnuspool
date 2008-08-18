/* readu.c -- read entry from user permissions file

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
#include "defaults.h"
#include "spuser.h"
#include "incl_unix.h"

/* Find user descriptor in file */

int	readu(const int fid, const uid_t uid, struct spdet * item)
{
	int_ugid_t	cuid = uid;

	/* If it's below the magic number at which we store them as a
	   vector, jump to the right place and go home.  */

	if  ((ULONG) cuid < SMAXUID)  {
		lseek(fid, (long)(sizeof(struct sphdr) + cuid * sizeof(struct spdet)), 0);
		if  (read(fid, (char *) item, sizeof(struct spdet)) == sizeof(struct spdet)  &&  item->spu_isvalid)
			return  1;
		return  0;
	}

	/* Otherwise seek to right place.  */

	lseek(fid, (long)(sizeof(struct sphdr) + sizeof(struct spdet) * SMAXUID), 0);
	while  (read(fid, (char *)item, sizeof(struct spdet)) == sizeof(struct spdet))  {
		if  ((ULONG) item->spu_user > (ULONG) cuid)
			break;
		if  (item->spu_user == cuid)
			return  item->spu_isvalid;
	}
	return  0;
}
