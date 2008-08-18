/*
 * Copyright (c) Xi Software Ltd. 1994.
 *
 * xt_ptrswap.c: created by John Collins on Fri Mar 11 1994.
 *----------------------------------------------------------------------
 * $Header: /sources/gnuspool/gnuspool/MSWIN/xtapi32/Xt_ptrsw.c,v 1.1 2008/08/18 16:25:54 jmc Exp $
 * $Log: Xt_ptrsw.c,v $
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

void xt_ptrswap(struct apispptr *to, const struct apispptr *from)
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
