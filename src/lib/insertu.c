/* insertu.c -- insert user details into user permissions file

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

/* Insert updated user descriptor into file.  */

void	insertu(const int fid, const struct spdet *item)
{
	LONG	pos;
	struct	spdet	*p1, *p2, *t;
	struct	spdet	a, b;

	/* If it's below maximum for vector, stuff it in.  */

	if  ((ULONG) item->spu_user < SMAXUID)  {
		pos = sizeof(struct sphdr) + item->spu_user * sizeof(struct spdet);
		goto  done;
	}

	/* Look for the user id.  */

	pos = sizeof(struct sphdr) + sizeof(struct spdet) * SMAXUID;

	lseek(fid, (long) pos, 0);
	while  (read(fid, (char *) &a, sizeof(struct spdet)) == sizeof(struct spdet))  {
		if  (a.spu_user == item->spu_user)
			break;
		if  ((ULONG) a.spu_user > (ULONG) item->spu_user)
			goto  slide;
		pos += sizeof(struct spdet);
	}
	goto  done;

	/* Passed the desired user id, so move all the others down.  */

 slide:
	p1 = &a;
	p2 = &b;

	while  (read(fid, (char *) p2, sizeof(struct spdet)) == sizeof(struct spdet))  {
		lseek(fid, -(long) sizeof(struct spdet), 1);
		write(fid, (char *) p1, sizeof(struct spdet));
		t = p1;
		p1 = p2;
		p2 = t;
	}
	write(fid, (char *) p1, sizeof(struct spdet));
 done:
	lseek(fid, (long) pos, 0);
	write(fid, (char *) item, sizeof(struct spdet));
}
