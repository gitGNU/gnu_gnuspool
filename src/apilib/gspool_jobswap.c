/* gspool_jobswap.c -- do all the byte-swapping we may need of job bits

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

#include <stdio.h>
#include <sys/types.h>
#include "gspool.h"
#include "xtapi_int.h"
#include "incl_unix.h"
#include "incl_net.h"

void	gspool_jobswap(struct apispq *to, const struct apispq *from)
{
	to->apispq_job = htonl((ULONG) from->apispq_job);
	to->apispq_netid = from->apispq_netid;
	to->apispq_orighost = from->apispq_orighost;
	to->apispq_rslot = htonl(from->apispq_rslot);
	to->apispq_time = htonl((ULONG) from->apispq_time);
	to->apispq_proptime = 0L;
	to->apispq_starttime = htonl((ULONG) from->apispq_starttime);
	to->apispq_hold = htonl((ULONG) from->apispq_hold);
	to->apispq_nptimeout = htons(from->apispq_nptimeout);
	to->apispq_ptimeout = htons(from->apispq_ptimeout);
	to->apispq_size = htonl((ULONG) from->apispq_size);
	to->apispq_posn = htonl((ULONG) from->apispq_posn);
	to->apispq_pagec = htonl((ULONG) from->apispq_pagec);
	to->apispq_npages = htonl((ULONG) from->apispq_npages);

	to->apispq_cps = from->apispq_cps;
	to->apispq_pri = from->apispq_pri;
	to->apispq_wpri = htons((USHORT) from->apispq_wpri);

	to->apispq_jflags = htons(from->apispq_jflags);
	to->apispq_sflags = from->apispq_sflags;
	to->apispq_dflags = from->apispq_dflags;

	to->apispq_extrn = htons((USHORT) from->apispq_extrn);
	to->apispq_pglim = htons((USHORT) from->apispq_pglim);

	to->apispq_class = htonl(from->apispq_class);
	to->apispq_pslot = htonl(from->apispq_pslot);

	to->apispq_start = htonl((ULONG) from->apispq_start);
	to->apispq_end = htonl((ULONG) from->apispq_end);
	to->apispq_haltat = htonl((ULONG) from->apispq_haltat);

	to->apispq_uid = htonl(from->apispq_uid);

	strncpy(to->apispq_uname, from->apispq_uname, UIDSIZE+1);
	strncpy(to->apispq_puname, from->apispq_puname, UIDSIZE+1);
	strncpy(to->apispq_file, from->apispq_file, MAXTITLE+1);
	strncpy(to->apispq_form, from->apispq_form, MAXFORM+1);
	strncpy(to->apispq_ptr, from->apispq_ptr, JPTRNAMESIZE+1);
	strncpy(to->apispq_flags, from->apispq_flags, MAXFLAGS+1);
}
