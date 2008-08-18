/* gspool_ptrswap.c -- do byte-swapping of printer attributes

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

void	gspool_ptrswap(struct apispptr *to, const struct apispptr *from)
{
	to->apispp_netid = from->apispp_netid;
	to->apispp_rslot = htonl((ULONG) from->apispp_rslot);

	to->apispp_pid = htonl((ULONG) from->apispp_pid);
	to->apispp_job = htonl((ULONG) from->apispp_job);
	to->apispp_rjhostid = from->apispp_rjhostid;
	to->apispp_rjslot = htonl((ULONG) from->apispp_rjslot);
	to->apispp_jslot = htonl((ULONG) from->apispp_jslot);

	to->apispp_state = from->apispp_state;
	to->apispp_sflags = from->apispp_sflags;
	to->apispp_dflags = from->apispp_dflags;
	to->apispp_netflags = from->apispp_netflags;

	to->apispp_class = htonl(from->apispp_class);

	to->apispp_minsize = htonl((ULONG) from->apispp_minsize);
	to->apispp_maxsize = htonl((ULONG) from->apispp_maxsize);

	to->apispp_extrn = htons(from->apispp_extrn);
	to->apispp_resvd = 0;

	strncpy(to->apispp_dev, from->apispp_dev, LINESIZE+1);
	strncpy(to->apispp_form, from->apispp_form, MAXFORM+1);
	strncpy(to->apispp_ptr, from->apispp_ptr, PTRNAMESIZE+1);
	strncpy(to->apispp_feedback, from->apispp_feedback, PFEEDBACK+1);
	strncpy(to->apispp_comment, from->apispp_comment, COMMENTSIZE+1);
}
