/*
 * Copyright (c) Xi Software Ltd. 1994.
 *
 * xt_jobswap.c: created by John Collins on Fri Mar 11 1994.
 *----------------------------------------------------------------------
 * $Header: /sources/gnuspool/gnuspool/MSWIN/xtapi32/XT_JOBSW.C,v 1.1 2008/08/18 16:25:54 jmc Exp $
 * $Log: XT_JOBSW.C,v $
 * Revision 1.1  2008/08/18 16:25:54  jmc
 * Initial revision
 *
 * Revision 22.1  1995/01/13  17:06:57  jmc
 * Brand New Release 22
 *
 * Revision 21.1  1994/08/31  18:22:26  jmc
 * Brand new Release 21
 *
 * Revision 20.1  1994/03/24  17:25:52  jmc
 * Brand new Release 20.
 *
 *----------------------------------------------------------------------
 */

#include <sys/types.h>
#include <string.h>
#include <winsock.h>
#include "xtapi.h"
#include "xtapi_in.h"

void	xt_jobswap(struct apispq *to, const struct apispq *from)
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
